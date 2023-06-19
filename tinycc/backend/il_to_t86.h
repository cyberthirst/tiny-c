//
// Created by Mirek Škrabal on 18.06.2023.
//

#pragma once

#include "../optimizer/il.h"
#include "t86_instruction.h"

/*
 * A visitor that translates the program in IR to T86.
 * It outputs a Program object that consists of T86 instruction class instances, it doesn't
 * output the assembly.
 */
namespace tiny {
    class T86CodeGen : public IRVisitor {
    public:
        static t86::Program translateProgram(Program const &program) {
            T86CodeGen gen;
            gen.generate(program);
            return std::move(gen.program_);
        }

        T86CodeGen & operator += (std::unique_ptr<t86::T86Ins> ins) {
            program_.addInstruction(std::move(ins));
            return *this;
        }
    private:
        void generate(Program const &program) {
            for (auto const &[name, function]: program.getFunctions()) {
                generateFunction(*function);
            }
        }

        void generateFunction(Function const &function) {
            for (auto const &basicBlock: function.getBasicBlocks()) {
                generateBasicBlock(*basicBlock);
            }
        }

        void generateBasicBlock(BasicBlock const &basicBlock) {
            for (size_t i = 0; i < basicBlock.size(); ++i) {
                Instruction *instruction = const_cast<Instruction*>(basicBlock[i]);
                visitChild(instruction);
            }
        }
        t86::Program program_;
    };

}