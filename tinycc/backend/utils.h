//
// Created by Mirek Škrabal on 11.06.2024.
//

#pragma once

#include <unordered_map>
#include <set>
#include <cassert>

#include "register.h"
#include "operand.h"
#include "program_structures.h"
#include "t86_instruction.h"

namespace tiny::t86 {

    bool isSpecialReg(Reg r) {
        return r == BP || r == SP || r == EAX;
    }

    struct OperandHash {
        std::size_t operator()(const Operand *operand) const {
            return operand->hash();
        }
    };

    struct OperandEqual {
        bool operator()(const Operand *lhs, const Operand *rhs) const {
            return lhs->equals(rhs);
        }
    };

    bool isSpecialRegOperand(Operand* operand) {
        auto reg = dynamic_cast<RegOp*>(operand);
        if (reg != nullptr && isSpecialReg(reg->reg_)) {
            assert(reg->reg_.physical());
            return true;
        }
        return false;
    }


    std::unordered_map<int, std::unordered_set<Operand*, OperandHash, OperandEqual>> computeLiveness(BasicBlock* block) {
        std::unordered_map<int, std::unordered_set<Operand*, OperandHash, OperandEqual>> liveness;
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
                if (dynamic_cast<CMPIns *>(binary) != nullptr) {
                    liveness[i].insert(binary->operand1_);
                    liveness[i].insert(binary->operand2_);
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

        return liveness;
    }


    bool isLastUse(std::unordered_map<int, std::unordered_set<Operand*, OperandHash, OperandEqual>> &liveness, Operand* operand, size_t i) {
        //for BP and SP we don't care about the last use
        if (isSpecialRegOperand(operand))
            return false;

        for (size_t j = i + 1; j < liveness.size(); ++j) {
            if (liveness[j].find(operand) != liveness[j].end())
                return false;
        }

        return true;
    }

    NOPIns * replaceWithNOP(BasicBlock* currentBlock_, size_t i) {
        assert(i < currentBlock_->size());
        auto nop = new NOPIns();
        currentBlock_->getInstructions()[i] = std::unique_ptr<Instruction>(nop);
        return nop;
    }

}
