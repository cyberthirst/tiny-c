//
// Created by Mirek Škrabal on 25.06.2023.
//

#pragma once

#include <set>

#include "register.h"
#include "program_structures.h"

namespace tiny::t86 {

    class RegAllocator {
    public:
        RegAllocator()
                : SP(Reg::Type::SP, INT_MAX),
                  BP(Reg::Type::BP, INT_MAX - 1),
                  EAX(Reg::Type::GP, 0) {}

        // allocates a free register on demand
        virtual Reg allocate() = 0;

        const Reg &getSP() const { return SP; }

        const Reg &getBP() const { return BP; }

        const Reg &getEAX() const { return EAX; }

    protected:
        Reg SP;  // Stack Pointer register
        Reg BP;  // Base Pointer register
        Reg EAX; // Return value register
    };


/*
 * Allocates abstract registers, ie ignores the limit set by the architecture.
 * Used during the instruction selection phase, the resulting target
 * code has to be further processed by a real register allocator.
 */
    class AbstractRegAllocator : public RegAllocator {
    public:
        AbstractRegAllocator()
                : RegAllocator() {
            //we reserve the REG0 as EAX for return values
            nextRegIndex_ = 1; // General Purpose registers start from 1
        }

        Reg allocate() override {
            return Reg(Reg::Type::GP, nextRegIndex_++);
        }

    private:
        int nextRegIndex_;
    };

/*
 * $$$$$$$$$$$$$$$ BASIC EXPLANATION $$$$$$$$$$$$$$$
 * 1. we need to keep track of liveness of variables
 *   - because we only consider local register allocation, this should be easy and we won't
 *     need any data flow analysis
 *     - local register allocation is concerned only with the current basic block, so we don't need
 *       to manage how the state transforms across different control flow paths
 * 2. we need to detect 2 things:
 *   a) variable definition - that is the assignment of a value to a variable
 *   b) variable use - that is the use of a variable in an expression
 * 3. definition of liveness of a variable:
 *   - a variable is live if it will be read in the future, ie the value is read before it is overwritten
 * 4. basic approach for computing the live variables:
 *   - start from the last instruction in the basic block and have empty set of live variables
 *   - then for each instruction:
 *     - remove target of the instruction
 *     - add all operands of the instruction
 *   - construct one set for each instruction
 * 5. Belady's algorithm:
 *  - when a variable is to be used it is assigned a physical register
 *  - however, when no physical registers are available, the variable with the furthest use is evicted
 *    from the register
 *    - that is the variable that is used furthest in the future will be spilled to memory
 *
 */


    class BeladyRegAllocator : public RegAllocator {
    public:
        static void allocatePhysicalRegs(Program &program, size_t numFreeRegs) {
            BeladyRegAllocator a(program, numFreeRegs);
            for (auto& [funName, function] : a.p_.getFunctions()) {
                auto &basicBlocks = function->getBasicBlocks();
                //allocate physical regs for each basic block
                for (auto &bb: basicBlocks) {
                    a.init(bb.get());
                    std::cout << "Starting allocation for block " << bb->name << std::endl;
                    a.allocate(bb.get());
                    a.deinit();
                }
            }
        }
    private:
        void init(BasicBlock* b) {
            computeLiveness(b);
        }

        void deinit() {
            liveness.clear();
            assert(freeRegs_.size() == numFreeRegs_);
            operandToRegMap_.clear();
        }


        BeladyRegAllocator(Program &program, size_t numFreeRegs) : p_(program), numFreeRegs_(numFreeRegs) {
            for (size_t i = 1; i <= numFreeRegs; ++i) {
                freeRegs_.insert(i);
            }
        }

        void computeLiveness(BasicBlock* block) {
            const auto& instructions = block->getInstructions();

            // Iterate over the instructions in reverse order
            for (int i = instructions.size() - 1; i >= 0; --i) {
                // copy the set of live vars from the next instruction (ie the one with the higher index)
                if (i < instructions.size() - 1)
                    liveness[i] = liveness[i + 1];

                const auto& instruction = instructions[i];
                const auto& binary = dynamic_cast<BinaryIns*>(instruction.get());
                // for binary insns (except CMP) we need to remove the target and add the source
                if (binary != nullptr) {
                    if (dynamic_cast<CMPIns *>(instruction.get()) != nullptr) {
                        for (const auto& operand : binary->getOperands())
                            liveness[i].insert(operand);
                        continue;
                    }
                    else {
                        auto target = binary->getOperands()[0];
                        auto source = binary->getOperands()[1];
                        liveness[i].erase(target);
                        liveness[i].insert(source);

                        continue;
                    }
                }

                for (const auto& operand : instruction->getOperands()) {
                    // don't care for labels
                    if (dynamic_cast<LabelOp*>(operand) != nullptr)
                        continue;

                    liveness[i].insert(operand);
                }
            }
        }

        Reg allocate() override {
            // If there are no free registers, spill one
            if (freeRegs_.empty()) {
                spillRegister();
            }

            size_t regId = *freeRegs_.begin();
            freeRegs_.erase(freeRegs_.begin());
            return Reg(Reg::Type::GP, regId, true);
        }

        void spillRegister() {
            assert(freeRegs_.empty());
            assert(curInsIndex < liveness.size());

            std::unordered_map<Operand*, Reg, OperandHash, OperandEqual> live = operandToRegMap_;
            assert(!live.empty());
            Operand * toSpill = nullptr;


            // Find the operand that is used furthest in the future

            for (size_t j = curInsIndex + 1; j < liveness.size(); ++j) {
                for (const auto& operand : liveness[j]) {
                    if (live.size() == 1) {
                        toSpill = live.begin()->first;
                        break;
                    }
                    else {
                        live.erase(operand);
                    }
                }
            }

            assert(toSpill != nullptr);

            // Spill the register
            // temporary solution assumes that the operand to be spilled corresponds to a stack variable
            MemRegOffsetOp *mem = dynamic_cast<MemRegOffsetOp*>(toSpill);
            assert(mem != nullptr);
            MOVIns *mov = new MOVIns(new MemRegOffsetOp(BP, mem->offset_),
                                     new RegOp(operandToRegMap_[toSpill]));
            currentBlock_->getInstructions().insert(currentBlock_->getInstructions().begin() + curInsIndex,
                                                    std::unique_ptr<Instruction>(mov));
            curInsIndex++;

            // Free the register
            freeRegs_.insert(operandToRegMap_[toSpill].index());
            operandToRegMap_.erase(toSpill);
        }

        bool isLastUse(Operand* operand, size_t i) {
            //for BP and SP we don't care about the last use
            if (isBPorSP(operand))
                return false;

            for (size_t j = i + 1; j < liveness.size(); ++j) {
                if (liveness[j].find(operand) != liveness[j].end())
                    return false;
            }

            return true;
        }

        struct OperandHash {
            std::size_t operator()(const Operand* operand) const {
                return operand->hash();
            }
        };

        struct OperandEqual {
            bool operator()(const Operand* lhs, const Operand* rhs) const {
                return lhs->equals(rhs);
            }
        };


        bool isBPorSP(Operand* operand) {
            auto reg = dynamic_cast<RegOp*>(operand);
            if (reg != nullptr && (reg->reg_ == getBP() || reg->reg_ == SP)) {
                return true;
            }
            return false;
        }


        void allocate(BasicBlock *b) {
            currentBlock_ = b;
            for (auto it = b->getInstructions().begin(); it != b->getInstructions().end(); ++it) {
                curInsIndex = it - b->getInstructions().begin();
                Instruction *i = it->get();

                if (dynamic_cast<MOVIns*>(i) != nullptr) {
                    auto operands = i->getOperands();
                    auto target = operands[0];
                    auto source = operands[1];
                    // source isn't in a register
                    if (operandToRegMap_.find(source) == operandToRegMap_.end()) {
                        Reg r = allocate();
                        assert(r.physical());

                        RegOp *targetOp = dynamic_cast<RegOp*>(target);
                        assert(targetOp != nullptr);
                        assert(!targetOp->reg_.physical());

                        //we allocated a register for the operand, update the mapping
                        operandToRegMap_[source] = r;

                        targetOp->reg_ = r;
                    }
                        // source is in a register -> eliminate the MOV
                    else {
                        // if source is in a register, target must be a physical register
                        assert(operandToRegMap_[target].physical());
                    }
                    std::cout << i->toString() << std::endl;
                    continue;
                }

                // We know that the operandToRegMap_ must contain the operands of the instruction
                // because they must have been preceeded by a MOV instruction
                // Because we use pointers for operands the registers should be remapped to
                // to physical automatically
                BinaryIns *binary = dynamic_cast<BinaryIns *>(i);
                if (binary != nullptr) {
                    std::vector<Operand *> originalOperands = binary->getOperands();
                    Reg &r1 = operandToRegMap_[i->getOperands()[0]];
                    assert(r1.physical());
                    Reg &r2 = operandToRegMap_[i->getOperands()[1]];
                    assert(r2.physical());

                    // If it is the last use of an operand, release the register
                    // Additionally, if the operand is in memory, we need to spill it
                    for (Operand *o: originalOperands) {
                        if (isLastUse(o, it - b->getInstructions().begin())) {
                            freeRegs_.insert(operandToRegMap_[o].index());
                            operandToRegMap_.erase(o);
                            // TODO
                            // If the operand is a register (and it's the last use), we should spill it
                            // although the spill should likely be done only for stack variables
                        }
                    }
                }
                std::cout << i->toString() << std::endl;
            }
        }

        BasicBlock *currentBlock_;
        size_t curInsIndex;
        std::unordered_map<Operand*, Reg, OperandHash, OperandEqual> operandToRegMap_;  // Map of operands to registers
        std::unordered_map<size_t, std::set<Operand*, OperandEqual>> liveness;
        // TODO represent free regs just with a simple counter
        std::set<int> freeRegs_;
        size_t numFreeRegs_;
        Program &p_;
    };


}