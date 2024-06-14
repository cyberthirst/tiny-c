//
// Created by Mirek Škrabal on 10.06.2024.
//

#pragma once

#include <functional>

#include "../backend/program_structures.h"
#include "../backend/utils.h"

namespace tiny {

    class RegOptimizer {
    public:
        static bool optimize(t86::Program p) {
            RegOptimizer o;
            bool changed = false;

            for (auto& [funName, function] : p.getFunctions()) {
                auto &basicBlocks = function->getBasicBlocks();
                for (auto &block: basicBlocks) {
                    o.setBlock(block.get());
                    for (const auto& rule : o.rules_) {
                        changed |= rule();
                    }
                }
            }

            return changed;
        }

    private:
        RegOptimizer() {
            rules_.emplace_back([this] { return rule_propageImmediates(); });
            rules_.emplace_back([this] { return rule_removeUnusedRegisters(); });
            rules_.emplace_back([this] { return rule_removeSpills(); });
            rules_.emplace_back([this] { return rule_removeFuncArgWrites(); });
        }

        // propagates immediate values
        bool rule_propageImmediates(){
            bool changed = false;
            auto liveness = computeLiveness(bb);

            for (size_t i = 0; i < bb->size(); ++i) {
                auto *movIns = dynamic_cast<t86::MOVIns *>((*bb)[i]);
                if (movIns == nullptr) continue;
                auto immOp = dynamic_cast<t86::ImmOp *>(movIns->operand2_);
                if (immOp == nullptr) continue;
                auto targetRegOp = dynamic_cast<t86::RegOp *>(movIns->operand1_);
                if (targetRegOp == nullptr) continue;
                for (size_t j = i + 1; j < bb->size(); ++j) {
                    if (liveness[j].find(targetRegOp) == liveness[j].end()) break;
                    auto *ins = (*bb)[j];
                    auto *binaryIns = dynamic_cast<t86::BinaryIns *>(ins);
                    if (binaryIns != nullptr) {
                        auto target = binaryIns->operand1_;
                        if (*target == *targetRegOp) break;
                        auto source = binaryIns->operand2_;
                        if (*source == *targetRegOp) {
                            binaryIns->operand2_ = immOp;
                            changed = true;
                        }
                    }
                    auto *unaryIns = dynamic_cast<t86::UnaryIns *>(ins);
                    if (unaryIns != nullptr) {
                        if (*unaryIns->operand_ == *targetRegOp) {
                            unaryIns->operand_ = immOp;
                            changed = true;
                        }
                    }
                }
            }

            return changed;
        }

        // removes unused registers
        bool rule_removeUnusedRegisters(){
            bool changed = false;
            auto liveness = computeLiveness(bb);
            for (size_t i = 0; i < bb->size(); ++i) {
                auto *movIns = dynamic_cast<t86::MOVIns *>((*bb)[i]);
                if (movIns == nullptr) continue;
                auto targetReg = dynamic_cast<t86::RegOp *>(movIns->operand1_);
                if (targetReg == nullptr) continue;
                if (t86::isSpecialRegOperand(targetReg)) continue;
                bool live = false;
                for (size_t j = i + 1; j < bb->size(); ++j) {
                    if (liveness[j].find(targetReg) != liveness[j].end()) {
                        live = true;
                        break;
                    }
                }
                if (!live) {
                    int addr = -1;
                    std::cout << bb->toString(addr);
                    std::cout << "index: " << i << " removing: " << movIns->toString() << "\n" << std::endl;
                    replaceWithNOP(bb, i);
                    changed = true;
                }
            }

            return changed;
        }

        // removes spills if value is not modified
        bool rule_removeSpills(){
            bool changed = false;

            return changed;
        }

        // due to how the regallocator works, we write also to the function arguments
        // but this doesn't have any semantical meaning w.r.t to the output of the program
        //  MOV [BP + 3], R2 <-- can be removed
        //  MOV [BP + 2], R1 <-- can be removed
        bool rule_removeFuncArgWrites() {
            bool changed = false;
            for (size_t i = 0; i < bb->size(); ++i) {
                auto *movIns = dynamic_cast<t86::MOVIns *>((*bb)[i]);
                if (movIns == nullptr) continue;
                auto target = movIns->operand1_;
                auto memOp = dynamic_cast<t86::MemRegOffsetOp *>(target);
                if (memOp == nullptr) continue;
                if (memOp->reg_ == t86::BP && memOp->offset_ > 0) {
                    replaceWithNOP(bb, i);
                    changed = true;
                }
            }
            return changed;
        }

        void setBlock(t86::BasicBlock *bb) {
            this->bb = bb;
        }

        t86::BasicBlock *bb;
        std::vector<std::function<bool()>> rules_;
    };
}