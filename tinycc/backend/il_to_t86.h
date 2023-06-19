//
// Created by Mirek Škrabal on 18.06.2023.
//

#pragma once

#include "../optimizer/il.h"

namespace tiny {
    class T86CodeGen : public IRVisitor {
    public:
        void generate(Program const &program) {
            for (auto const &[name, function]: program.functions_) {
                generateFunction(*function);
            }
        }

    private:
        void generateFunction(Function const &function) {
            for (auto const &basicBlock: function.bbs_) {
                generateBasicBlock(*basicBlock);
            }
        }

        void generateBasicBlock(BasicBlock const &basicBlock) {
            for (size_t i = 0; i < basicBlock.size(); ++i) {
                Instruction *instruction = const_cast<Instruction*>(basicBlock[i]);
                visitChild(instruction);
            }
        }

    };
}