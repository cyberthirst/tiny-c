//
// Created by Mirek Škrabal on 29.05.2024.
//

#pragma once

#include <vector>

#include "../backend/program_structures.h"
#include "../backend/t86_instruction.h"

/*
 * patterns
 * MOV R1, 10
 * MOV [BP - 1], R1 <-- replace R1 with 10, could be useful when R1 is not used anymore (so
 *   in liveness analysis we can remove it)
 *
 * ADD R1, 0
 *
 * MOV R1, R1
 *
 * MOV R1, [BP - 1]
 * MOV [BP - 1], R1 <-- replace with NOP
 *
 * MOV [BP - 1], 0 <-- replace with NOP (value is not used - it is overwritten in the next mov)
 * MOV [BP - 1], R2
 *
 * NOP
 *
 * JUMP target, where target is current instruction + 1
 */

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
            }
            while (bbIndex_ < bbs_->size() && bbs_->operator[](bbIndex_)->size() == 0) {
                bbIndex_++;
            }
            instrIndex_++;

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
        std::pair<size_t, size_t> getNthIndices(size_t n) {
            size_t bb = bbIndex_;
            size_t instr = instrIndex_;
            while (n > 0) {
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
            assert(bb < bbs_->size());
            assert(instr < bbs_->operator[](bb)->size());
            assert(bbs_->operator[](bb)->operator[](instr) == expected);
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
            std::cout << "bbIndex: " << bbIndex_ << " instrIndex: " << instrIndex_ << ": ";
            auto bb = bbs_->operator[](bbIndex_).get();
            std::cout << bb->operator[](instrIndex_)->toString() << std::endl;
        }

    private:
        std::vector<std::unique_ptr<t86::BasicBlock>> *bbs_;
        size_t windowBBIndex_ = 0;
        size_t windowInstrIndex_ = 0;
        size_t bbIndex_ = 0;
        size_t instrIndex_ = 0;

    };


    class PeepholeOptimizer : public ProgramTraverser{
    public:
        static bool optimize(t86::Program &program) {
            PeepholeOptimizer o(program);

            bool changed = false;

            for (auto &f : program.getFunctions()) {
                o.setFunction(f.second);
                for (auto &rule : o.rules_) {
                    while (!o.isEnd()) {
                        //o.print();
                        if (rule()) {
                            changed = true;
                        }
                        o.shift();
                    }
                }
            }

            return changed;
        }

    private:
        PeepholeOptimizer(t86::Program &p){
            //rules_.emplace_back([this] { return this->rule_removeAddZero(); });
            rules_.emplace_back([this] { return this->rule_removeNOP(); });
        }

        void remove(size_t n, t86::Instruction *expected) {
            auto [bb, instr] = getNthIndices(n);
            removeInstruction(bb, instr, expected);
        }


        bool rule_removeAddZero() {
            auto i = getInstruction();
            auto addIns = dynamic_cast<t86::ADDIns *>(i);
            if (addIns == nullptr) return false;
            auto source = addIns->operand2_;
            auto imm = dynamic_cast<t86::ImmOp *>(source);
            if (imm == nullptr) return false;
            if (imm->value_ == 0) {
                remove(0, i);
                return true;
            }
            return false;
        }

        bool rule_removeNOP() {
            auto i = getInstruction();
            auto nopIns = dynamic_cast<t86::NOPIns *>(i);
            if (nopIns == nullptr) return false;
            remove(0, i);
            return true;

        }

        std::vector<std::function<bool()>> rules_;

    };
}