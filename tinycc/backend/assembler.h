//
// Created by Mirek Škrabal on 23.06.2023.
//

#pragma once

#include <map>

#include "program_structures.h"
#include "constants.h"

namespace tiny {
    class Assembler {
    public:
        static void assemble(t86::Program &program) {
            Assembler a;
            a.assembleProgram(program);
        }
    private:
        //calculates the addresses of the functions and basic blocks
        void firstPass(t86::Program &program) {
            sizeOfProgram = 0;
            //TODO here we assume that the first iterated function is main, which is
            //currently true, but is fragile and might change in the future
            for(auto& [funName, function] : program.getFunctions()) {
                labelAddressMap[funName.name()] = sizeOfProgram;
                for (auto &block : function->getBasicBlocks()) {
                    labelAddressMap[block->name] = sizeOfProgram;
                    sizeOfProgram += block->size();
                }
            }
        }


        void secondPass(t86::Program &program) {
            for (auto& [funName, function] : program.getFunctions()) {
                auto &basicBlocks = function->getBasicBlocks();
                for (auto &block: basicBlocks) {
                    for (size_t i = 0; i < block->size(); i++) {
                        auto *ins = (*block)[i];
                        //TODO might be better to use the opcode instead of dynamic_cast
                        //opcodes aren't currently implemented
                        auto *jumpIns = dynamic_cast<t86::JumpIns *>(ins);
                        if (jumpIns) {
                            int address = labelAddressMap[jumpIns->lbl_->toString()];
                            jumpIns->patchLabel(address);
                        }

                        auto *callIns = dynamic_cast<t86::CALLIns *>(ins);
                        if (callIns) {
                            int address = labelAddressMap[callIns->lbl_->toString()];
                            jumpIns->patchLabel(address);
                        }
                    }
                }
            }
        }

        void assembleProgram(t86::Program &program) {
            firstPass(program);
            secondPass(program);
        }

        //size of the program in instructions
        size_t sizeOfProgram = INS_START_FUN_SZ;

        std::map<std::string, size_t> labelAddressMap;
    };
}