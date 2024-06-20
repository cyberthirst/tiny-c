//
// Created by Mirek Škrabal on 29.05.2024.
//

#pragma once

#include <vector>

#include "../backend/program_structures.h"
#include "../backend/t86_instruction.h"


namespace tiny {

    class ProgramTraverser {

    public:
        ProgramTraverser() = default;

        void setFunction(t86::Function *f) {
            bbs_ = &f->getBasicBlocks();
            bbIndex_ = 0;
            instrIndex_ = 0;
            windowBBIndex_ = 0;
            windowInstrIndex_ = 0;
        }

        void shift() {
            if (instrIndex_ + 1 >= bbs_->operator[](bbIndex_)->size()) {
                instrIndex_ = 0;
                bbIndex_++;
                while (bbIndex_ < bbs_->size() && bbs_->operator[](bbIndex_)->size() == 0) {
                    bbIndex_++;
                }
            }
            else {
                instrIndex_++;
            }

            totalInstrs_++;
            windowBBIndex_ = bbIndex_;
            windowInstrIndex_ = instrIndex_;
        }

        void resetWindow() {
            windowBBIndex_ = bbIndex_;
            windowInstrIndex_ = instrIndex_;
        }

        t86::Instruction * getInstruction() {
            if (windowInstrIndex_ >= bbs_->operator[](windowBBIndex_)->size()) {
                windowBBIndex_++;
                windowInstrIndex_ = 0;
            }
            if (windowBBIndex_ >= bbs_->size()) {
                return nullptr;
            }

            return bbs_->operator[](windowBBIndex_)->operator[](windowInstrIndex_++);
        }

        // get indexes of the nth instruction
        // n represents position in the window
        std::pair<size_t, size_t> getIndexesForNthInstr(size_t n) {
            size_t bb = bbIndex_;
            size_t instr = instrIndex_;
            while (n > 1) {
                if (instr >= bbs_->operator[](bb)->size()) {
                    instr = 0;
                    bb++;
                }
                else {
                    instr++;
                }
                n--;
            }

            return {bb, instr};
        }

        void removeInstruction(size_t bb, size_t instr, t86::Instruction *expected ) {
            auto block = bbs_->operator[](bb).get();
            assert(bb < bbs_->size());
            assert(instr < block->size());
            assert(expected == block->operator[](instr));
            auto begin = bbs_->operator[](bb)->getInstructions().begin();
            bbs_->operator[](bb)->getInstructions().erase(begin + instr);
            if (bbs_->operator[](bb)->size() == 0) {
                bbs_->erase(bbs_->begin() + bb);
            }
        }

        bool isEnd() {
            return bbIndex_ >= bbs_->size();
        }

        void print(){
            std::cout << bbs_->operator[](bbIndex_)->name << ": " << "bbIndex: " << bbIndex_ << " instrIndex: " << instrIndex_ << ": ";
            auto bb = bbs_->operator[](bbIndex_).get();
            std::cout << bb->operator[](instrIndex_)->toString() << std::endl;
        }

    private:
        std::vector<std::unique_ptr<t86::BasicBlock>> *bbs_;
        size_t windowBBIndex_ = 0;
        size_t windowInstrIndex_ = 0;
        size_t bbIndex_ = 0;
        size_t instrIndex_ = 0;
        size_t totalInstrs_ = 0;

    };


    class PeepholeOptimizer : public ProgramTraverser {
    public:
        static bool optimize(t86::Program &program) {
            PeepholeOptimizer o(program);

            bool changed = false;

            for (auto &f : program.getFunctions()) {
                o.setFunction(f.second);
                    while (!o.isEnd()) {
                        for (auto &rule : o.rules_) {
                            //o.print();
                            changed |= rule();
                            o.resetWindow();
                        }
                        o.shift();
                    }
            }

            return changed;
        }

    private:
        PeepholeOptimizer(t86::Program &p){
            rules_.emplace_back([this] { return this->rule_removeAddSubZero(); });
            rules_.emplace_back([this] { return this->rule_removeNOP(); });
            rules_.emplace_back([this] { return this->rule_removeSelfCopy(); });
            rules_.emplace_back([this] { return this->rule_removeUnusedMov(); });
            rules_.emplace_back([this] { return this->rule_removeCyclicMov(); });
        }

        void remove(size_t n, t86::Instruction *expected) {
            auto [bb, instr] = getIndexesForNthInstr(n);
            removeInstruction(bb, instr, expected);
        }

        // removes patterns like: ADD R1, 0 or SUB R1, 0
        bool rule_removeAddSubZero() {
            auto i = getInstruction();
            auto addIns = dynamic_cast<t86::ADDIns *>(i);
            auto subIns = dynamic_cast<t86::SUBIns *>(i);
            if (addIns == nullptr && subIns == nullptr) return false;
            auto binary = dynamic_cast<t86::BinaryIns *>(i);
            assert(binary != nullptr);
            auto source = binary->operand2_;
            auto imm = dynamic_cast<t86::ImmOp *>(source);
            if (imm == nullptr) return false;
            if (imm->value_ == 0) {
                remove(0, i);
                return true;
            }
            return false;
        }

        // removes NOP instructions
        bool rule_removeNOP() {
            auto i = getInstruction();
            auto nopIns = dynamic_cast<t86::NOPIns *>(i);
            if (nopIns == nullptr) return false;
            remove(0, i);
            return true;

        }

        // removes patterns like: MOV R1, R1
        bool rule_removeSelfCopy() {
            auto i = getInstruction();
            auto movIns = dynamic_cast<t86::MOVIns *>(i);
            if (movIns == nullptr) return false;
            if (*movIns->operand1_ == *movIns->operand2_) {
                remove(0, i);
                return true;
            }
            return false;
        }


        // removes unused consequent MOV instructions to the same target
        // MOV [BP - 1], 0  <-- can be removed
        // MOV [BP - 1], R2
        bool rule_removeUnusedMov() {
            auto i = getInstruction();
            auto movIns = dynamic_cast<t86::MOVIns *>(i);
            if (movIns == nullptr) return false;
            auto target = movIns->operand1_;
            auto next = getInstruction();
            auto nextMovIns = dynamic_cast<t86::MOVIns *>(next);
            if (nextMovIns == nullptr) return false;
            if (*nextMovIns->operand1_ == *target) {
                remove(0, i);
            }
            return false;
        }

        // removes cyclic MOV instructions
        // MOV R1, [BP - 1]
        // MOV [BP - 1], R1 <-- can be removed
        bool rule_removeCyclicMov() {
            auto i = getInstruction();
            auto movIns = dynamic_cast<t86::MOVIns *>(i);
            if (movIns == nullptr) return false;
            auto source = movIns->operand2_;
            auto next = getInstruction();
            auto nextMovIns = dynamic_cast<t86::MOVIns *>(next);
            if (nextMovIns == nullptr) return false;
            if (*nextMovIns->operand1_ == *source) {
                remove(1, i);
            }
            return false;
        }


        std::vector<std::function<bool()>> rules_;

    };
}