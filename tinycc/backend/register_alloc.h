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
 * plan of attack
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
    private:

    };

    class RndRegAllocator : public RegAllocator {
    public:
        static void allocatePhysicalRegs(Program &program, size_t numFreeRegs) {
            RndRegAllocator a(program, numFreeRegs);
            for (auto& [funName, function] : a.p_.getFunctions()) {
                auto &basicBlocks = function->getBasicBlocks();
                for (auto &bb: basicBlocks) {
                    a.computeDefUseChain(bb.get());
                    a.allocate(bb.get());
                    a.defUseChain_.clear();
                }
            }
        }
    private:
        RndRegAllocator(Program &program, size_t numFreeRegs) : p_(program) {
            for (size_t i = 1; i <= numFreeRegs; ++i) {
                freeRegs_.insert(i);
            }
        }

        void computeDefUseChain(BasicBlock* block) {
            const auto& instructions = block->getInstructions();

            // For each instruction
            for (size_t i = 0; i < instructions.size(); ++i) {
                const auto& instruction = instructions[i];

                // For each operand in the instruction
                for (const auto& operand : instruction->getOperands()) {
                    // Add the current instruction and its index to the def-use chain of the operand
                    defUseChain_[operand].push_back(std::make_pair(instruction.get(), i));
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
            size_t spillRegId = *(std::next(freeRegs_.begin(), rand() % freeRegs_.size()));
            freeRegs_.erase(spillRegId);

            // TODO: You should handle moving the contents of this spilled register to memory.
        }

        bool isLastUse(Instruction *instr, Operand* operand) {
            //for BP and SP we don't care about the last use
            auto reg = dynamic_cast<RegOp*>(operand);
            if (reg != nullptr && (reg->reg_ == getBP() || reg->reg_ == SP)) {
                return false;
            }
            auto &uses = defUseChain_[operand];

            // Check if the instruction 'instr' is the last element in the vector of uses.
            // The instruction pointer and the index (i) of the operand should match.
            return std::find_if(uses.rbegin(), uses.rend(), [&](const auto &pair) {
                return pair.first == instr;
            }) == uses.rbegin();
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

        void allocate(BasicBlock *b) {
            std::unordered_map<Operand*, Reg> operandToRegMap;  // Map of operands to registers
            std::unordered_set<Operand*, OperandHash, OperandEqual> memoryOperands; // Set of operands which are in memory

            // Initialize memoryOperands
            for (auto& instr : b->getInstructions()) {
                for (Operand* op : instr->getOperands()) {
                    if (dynamic_cast<ImmOp*>(op) != nullptr || dynamic_cast<MemRegOffsetOp*>(op) != nullptr) {
                        memoryOperands.insert(op);
                    }
                }
            }

            for (auto it = b->getInstructions().begin(); it != b->getInstructions().end(); ++it) {
                Instruction *i = it->get();

                for (Operand* o : i->getOperands()) {
                    if (memoryOperands.find(o) != memoryOperands.end()) {
                        Reg r = allocate();
                        operandToRegMap[o] = r;

                        // operand is in memory, so we need to load it into a register
                        auto moveInstr = std::make_unique<MOVIns>(new RegOp(r), o);

                        // insert MOV instruction into the basic block
                        // currently this is very stupid & inefficient the insert has O(n) complexity, using it for simplicity
                        it = b->getInstructions().insert(it, std::move(moveInstr));

                        // Since we inserted a new instruction, the iterator positions have changed.
                        // We need to increment 'it' to point to the original instruction.
                        ++it;
                    }
                }

                // If it is the last use of an operand, release the register
                for (Operand* o : i->getOperands()) {
                    if (isLastUse(i, o)) {
                        //if the operand is in a register, we can free it
                        if (operandToRegMap.find(o) != operandToRegMap.end()) {
                            freeRegs_.insert(operandToRegMap[o].index());
                            operandToRegMap.erase(o);
                        }
                    }
                }


                // we are defining a value, so we need to allocate a register for it
                auto mov = dynamic_cast<MOVIns*>(i);
                if (mov != nullptr && mov->operand1_) {
                    Reg r = allocate();
                    //update the abstract register with the physical register
                    mov->operand1_ = new RegOp(r);
                    operandToRegMap[mov->operand1_] = r;
                }
            }
        }



        std::unordered_map<Operand *, std::vector<std::pair<Instruction *, unsigned long>>> defUseChain_;
        std::set<size_t> freeRegs_;
        Program &p_;
    };
}