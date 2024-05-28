//
// Created by Mirek Škrabal on 25.06.2023.
//

#pragma once

#include <set>

#include "register.h"
#include "program_structures.h"

namespace tiny::t86 {
    using DefUseChain = std::unordered_map<Operand *, std::vector<std::pair<Instruction *, size_t>>>;

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


    class LocalRegAllocator : public RegAllocator {
    public:
        static void allocatePhysicalRegs(Program &program, size_t numFreeRegs) {
            LocalRegAllocator a(program, numFreeRegs);
            for (auto& [funName, function] : a.p_.getFunctions()) {
                auto &basicBlocks = function->getBasicBlocks();
                //allocate physical regs for each basic block
                for (auto &bb: basicBlocks) {
                    a.init(bb.get());
                    a.allocate(bb.get());
                    a.deinit();
                }
            }
        }
    private:
        void init(BasicBlock* b) {
            computeLiveness(b);
            // Initialize memoryOperands_
            for (auto& instr : b->getInstructions()) {
                for (Operand* op : instr->getOperands()) {
                    //if (dynamic_cast<ImmOp*>(op) != nullptr || dynamic_cast<MemRegOffsetOp*>(op) != nullptr) {
                   if (dynamic_cast<MemRegOffsetOp*>(op) != nullptr) {
                        memoryOperands_.insert(op);
                    }
                }
            }
        }

        void deinit() {
            liveness.clear();
            freeRegs_.clear();
            operandToRegMap_.clear();
            memoryOperands_.clear();
        }


        LocalRegAllocator(Program &program, size_t numFreeRegs) : p_(program) {
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
                // for MOVs, we set the source as live, and remove the destination from live
                // TODO arithmetic instructions also invalidate the destination (and possibly also other insns)
                if (dynamic_cast<MOVIns*>(instruction.get()) != nullptr) {
                    auto target = instruction->getOperands()[0];
                    auto source = instruction->getOperands()[1];
                    liveness[i].erase(target);
                    liveness[i].insert(source);

                    continue;
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
            return Reg(Reg::Type::GP, regId);
        }

        void spillRegister() {
            /*size_t spillRegId = *(std::next(freeRegs_.begin(), rand() % freeRegs_.size()));
            freeRegs_.erase(spillRegId);*/
            // 1. we need to add the instruction to move the operand to memory
            // 2. we need to remove the register from the operandToReg_ map
            // 3. we need to add the reg
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

        Reg* findReg(Operand* operand) {
            auto it = operandToRegMap_.find(operand);
            if (it != operandToRegMap_.end()) {
                // The operand is already present in the map.
                return &(it->second);
            } else {
                // The operand isn't present in the map.
                return nullptr;
            }
        }

        bool isBPorSP(Operand* operand) {
            auto reg = dynamic_cast<RegOp*>(operand);
            if (reg != nullptr && (reg->reg_ == getBP() || reg->reg_ == SP)) {
                return true;
            }
            return false;
        }

        void updateLivenessAndInsns(BasicBlock *b, size_t i, Operand *original, Operand *replacement) {
            // update all the instructions that use the original operand
            for (auto it = b->getInstructions().begin() + i; it != b->getInstructions().end(); ++it) {

            }

            // update the liveness of the original operand
            // - remove the original operand from the liveness set and add the replacement
        }

        void allocate(BasicBlock *b) {
            for (auto it = b->getInstructions().begin(); it != b->getInstructions().end(); ++it) {
                Instruction *i = it->get();

                if (dynamic_cast<MOVIns*>(i) != nullptr) {
                    auto operands = i->getOperands();
                    auto target = operands[0];
                    auto source = operands[1];
                    // source isn't in a register
                    if (operandToRegMap_.find(source) == operandToRegMap_.end()) {
                        Reg r = allocate();

                        //we allocated a register for the operand, update the mapping
                        operandToRegMap_[source] = r;

                        auto newOp = new RegOp(r);

                        // update the MOV with the new operand
                        MOVIns *mov = dynamic_cast<MOVIns *>(i);
                        mov->operand2_ = newOp;
                    }
                        // source is in a register -> eliminate the MOV
                    else {
                        // we don't need to do anything, the source is already in a register
                        // so rewrite to NOP
                        NOPIns *nop = new NOPIns();
                        *it = std::unique_ptr<Instruction>(nop);
                    }
                    continue;
                }

                // We know that the operandToRegMap_ must contain the operands of the instruction
                // because they must have been preceeded by a MOV instruction
                // But they still use the original operands, so we need to remap them to the physical register
                // operands
                BinaryIns *binary = dynamic_cast<BinaryIns *>(i);
                if (binary != nullptr) {
                    std::vector<Operand *> originalOperands = binary->getOperands();
                    Reg &r1 = operandToRegMap_[i->getOperands()[0]];
                    Reg &r2 = operandToRegMap_[i->getOperands()[1]];

                    binary->operand1_ = new RegOp(r1);
                    binary->operand2_ = new RegOp(r2);


                    // If it is the last use of an operand, release the register
                    // Additionally, if the operand is in memory, we need to spill it
                    for (Operand *o: originalOperands) {
                        if (isLastUse(o, it - b->getInstructions().begin())) {
                            freeRegs_.insert(operandToRegMap_[o].index());
                            operandToRegMap_.erase(o);
                            // If the operand is in memory, we need to spill it
                            MemRegOffsetOp *mem = dynamic_cast<MemRegOffsetOp *>(o);
                            if (mem != nullptr) {
                                // TODO spill the operand
                            }
                        }
                    }
                }
            }
        }

        std::unordered_map<Operand*, Reg, OperandHash, OperandEqual> operandToRegMap_;  // Map of operands to registers
        std::unordered_set<Operand*, OperandHash, OperandEqual> memoryOperands_; // Set of operands which are in memory
        std::unordered_map<size_t, std::set<Operand*, OperandEqual>> liveness;
        // TODO represent free regs just with a simple counter
        std::set<size_t> freeRegs_;
        Program &p_;
    };


}