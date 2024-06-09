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
                : SP(Reg::Type::SP, INT_MAX, true),
                  BP(Reg::Type::BP, INT_MAX - 1, true),
                  EAX(Reg::Type::GP, 0, true) {}

        // allocates a free register on demand
        virtual Reg allocate() = 0;

        const Reg &getSP() const { assert(SP.physical()); return SP; }

        const Reg &getBP() const { assert(BP.physical()); return BP; }

        const Reg &getEAX() const { assert(EAX.physical()); return EAX; }

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
                    //a.printFreeRegs();
                    // TODO add constant propagation
                    // TODO remove unused registers
                    // TODO remove spills if value wasn't modified after the initial load
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

        void printFreeRegs() {
            std::cout << "Free registers: ";
            for (auto it : freeRegs_) {
                std::cout << it << " ";
            }
            std::cout << std::endl;
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

        void insertInsBeforeCurrent(Instruction *ins) {
            auto &instructions = currentBlock_->getInstructions();
            instructions.insert(instructions.begin() + curInsIndex, std::unique_ptr<Instruction>(ins));
            std::cout << instructions[curInsIndex]->toString() << std::endl;
            // we need to update liveness of subsequent instructions and liveness is mapping
            // from instruction index to set of operands
            for (size_t i = liveness.size(); i > curInsIndex; --i) {
                liveness[i] = liveness[i - 1];
            }
            // update the liveness of the current instruction
            liveness[curInsIndex] = liveness[curInsIndex + 1];
            curInsIndex++;
        }

        void spillHelper(Operand *toSpill){
            auto *mem = dynamic_cast<MemRegOffsetOp*>(toSpill);
            assert(mem != nullptr);
            MOVIns *mov = new MOVIns(new MemRegOffsetOp(BP, mem->offset_),
                                     new RegOp(operandToRegMap_[toSpill]));
            insertInsBeforeCurrent(mov);

            // Free the register
            insertFreeReg(operandToRegMap_[toSpill]);
            operandToRegMap_.erase(toSpill);
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
            spillHelper(toSpill);
        }

        bool isLastUse(Operand* operand, size_t i) {
            //for BP and SP we don't care about the last use
            if (isSpecialRegOperand(operand))
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

        bool isSpecialReg(Reg r) {
            return r == getBP() || r == getSP() || r == getEAX();
        }


        bool isSpecialRegOperand(Operand* operand) {
            auto reg = dynamic_cast<RegOp*>(operand);
            if (reg != nullptr && isSpecialReg(reg->reg_)) {
                assert(reg->reg_.physical());
                return true;
            }
            return false;
        }

        void printOperandToRegMap() {
            std::cout << "->->->->->->->->->->" << std::endl;
            for (const auto& [operand, reg] : operandToRegMap_) {
                std::cout << operand->toString() << " -> " << reg.toString() << std::endl;
            }
            std::cout << "<-<-<-<-<-<-<-<-<-<-<-" << std::endl;
        }

        void insertFreeReg(Reg r) {
            assert(r.physical());
            assert(!isSpecialReg(r));
            //assert(freeRegs_.find(r.index()) == freeRegs_.end());
            freeRegs_.insert(r.index());
        }

        void finalizeBB() {
            //printFreeRegs();
            for (auto it = operandToRegMap_.begin(); it != operandToRegMap_.end();) {
                auto *memOp = dynamic_cast<MemRegOffsetOp*>(it->first);
                if (memOp != nullptr) {
                    auto currentIt = it; // store current iterator
                    ++it; // move iterator to next element before potential modification
                    spillHelper(currentIt->first);
                } else {
                    auto reg = it->second;
                    if (!isSpecialReg(reg)) insertFreeReg(reg);
                    ++it;
                }
            }
            //printFreeRegs();
        }

        void physicalRegInvariant() {
            for (const auto& [operand, reg] : operandToRegMap_) {
                assert(reg.physical());
            }
        }

        void changeMappingOfOperands(Reg oldReg, Reg newReg) {
            for (auto& [operand, reg] : operandToRegMap_) {
                if (reg == oldReg) {
                    reg = newReg;
                }
            }
        }

        void remapOperands(BinaryIns *binary) {
            std::vector<Operand *> originalOperands = binary->getOperands();
            auto target = originalOperands[0];
            auto source = originalOperands[1];

            auto targetRegOp = dynamic_cast<RegOp*>(target);
            if (targetRegOp != nullptr && !isSpecialReg(targetRegOp->reg_)) {
                assert(operandToRegMap_.find(target) != operandToRegMap_.end());
                assert(operandToRegMap_[target].physical());
                // binary operations overwrite the target
                // but the target might be used in the future - so we need to move it
                if (dynamic_cast<CMPIns*>(binary) == nullptr) {
                    // check if there exists an operand which maps to the same register as the target
                    // and which is different from the target and which is not the last use
                    // if such operand exists, we need to move it to a different register
                    bool found = false;
                    for (auto& [operand, reg] : operandToRegMap_) {
                        if (operand != target) {
                            if (reg == operandToRegMap_[target]) {
                                if (!isLastUse(operand, curInsIndex)){
                                    found = true;
                                }
                            }
                        }
                    }
                    if (found) {
                        auto reg = allocate();
                        assert(reg.physical());
                        auto mov = new MOVIns(new RegOp(reg), new RegOp(operandToRegMap_[target]));
                        insertInsBeforeCurrent(mov);
                        //changeMappingOfOperands(operandToRegMap_[target], reg);
                        operandToRegMap_[target] = reg;
                    }

                }
                binary->operand1_ = new RegOp(operandToRegMap_[target]);
            }

            auto sourceRegOp = dynamic_cast<RegOp*>(source);
            if (sourceRegOp != nullptr) {
                // TODO make into a function
                assert(operandToRegMap_.find(source) != operandToRegMap_.end());
                assert(operandToRegMap_[source].physical());
                binary->operand2_ = new RegOp(operandToRegMap_[source]);
            }
        }


        NOPIns * replaceCurrentInsWithNOP() {
            auto nop = new NOPIns();
            currentBlock_->getInstructions()[curInsIndex] = std::unique_ptr<Instruction>(nop);
            return nop;
        }


        void allocate(BasicBlock *b) {
            currentBlock_ = b;
            assert(operandToRegMap_.empty());
            auto &instructions = b->getInstructions();
            for (curInsIndex = 0; curInsIndex < instructions.size(); ++curInsIndex) {
                Instruction *i = instructions[curInsIndex].get();
                physicalRegInvariant();
                //std::cout << "Processing instruction " << i->toString() << std::endl;
                //printOperandToRegMap();

                // last instruction in the block
                if (curInsIndex + 1 == instructions.size()) {
                    assert(dynamic_cast<NoOpIns*>(i) != nullptr || dynamic_cast<JumpIns*>(i) != nullptr);
                    finalizeBB();
                }

                auto mov = dynamic_cast<MOVIns*>(i);
                if (mov != nullptr) {
                    auto operands = mov->getOperands();
                    auto target = mov->operand1_;
                    auto source = mov->operand2_;

                    // for special registers like SP, BP, EAX we don't care about allocation and
                    // use the instruction as is
                    if (isSpecialRegOperand(target) || isSpecialRegOperand(source)) {
                        for (Operand *o: operands) {
                            if (isSpecialRegOperand(o)) {
                                auto reg = dynamic_cast<RegOp*>(o);
                                operandToRegMap_[o] = reg->reg_;
                                if (reg->reg_ == getEAX()){
                                    assert(operandToRegMap_.find(source) != operandToRegMap_.end());
                                    mov->operand2_ = new RegOp(operandToRegMap_[source]);
                                }
                            }
                        }
                        std::cout << i->toString() << std::endl;
                        continue;
                    }

                    // source is not in register
                    else if (operandToRegMap_.find(source) == operandToRegMap_.end()) {
                        Reg r = allocate();
                        assert(r.physical());

                        auto targetMem = dynamic_cast<MemRegOffsetOp*>(target);
                        if (targetMem != nullptr) {
                            // replace the memory operand with a register operand
                            // and map the memory operand to the register
                            operandToRegMap_[source] = r;
                            operandToRegMap_[target] = r;
                            mov->operand1_ = new RegOp(r);

                        }
                        else {
                            RegOp *targetOp = dynamic_cast<RegOp *>(target);
                            assert(targetOp != nullptr);
                            assert(!targetOp->reg_.physical());

                            //we allocated a register for the operand, update the mapping
                            operandToRegMap_[source] = r;
                            // TODO this is little sus
                            // maybe we should check if operand is either mapped or its a physical register?
                            operandToRegMap_[target] = r;
                            mov->operand1_ = new RegOp(r);
                        }
                    }
                    else { //source is in register
                        auto memOp = dynamic_cast<MemRegOffsetOp*>(target);
                        // target is memory and source is in a register
                        // do the optimization to replace the MOV with NOP
                        if (memOp != nullptr) { // target is memory, thus we replace the MOV with NOP
                            if (operandToRegMap_.find(target) != operandToRegMap_.end())
                                insertFreeReg(operandToRegMap_[target]);
                            operandToRegMap_[target] = operandToRegMap_[source];
                            i = replaceCurrentInsWithNOP(); // assign is done just for printing purposes
                        }
                        // source is in register and target is a register
                        // in this case the source can be in more than one register (bc we map it to another register)
                        // but we still map it only to one register, this might be problematic
                        else {
                            operandToRegMap_[target] = operandToRegMap_[source];
                            //operandToRegMap_[source] = operandToRegMap_[target];
                            i = replaceCurrentInsWithNOP(); // assign is done just for printing purposes
                            assert(operandToRegMap_[target].physical());
                        }
                    }
                    std::cout << i->toString() << std::endl;
                    continue;
                }

                BinaryIns *binary = dynamic_cast<BinaryIns *>(i);
                if (binary != nullptr)
                    remapOperands(binary);
                std::cout << i->toString() << std::endl;
            }

        }

        BasicBlock *currentBlock_;
        size_t curInsIndex;
        std::unordered_map<Operand*, Reg, OperandHash, OperandEqual> operandToRegMap_;  // Map of operands to registers
        std::unordered_map<size_t, std::set<Operand*, OperandEqual>> liveness;
        std::set<int> freeRegs_;
        size_t numFreeRegs_;
        Program &p_;
    };


}