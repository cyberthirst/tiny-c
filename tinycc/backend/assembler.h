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
        Assembler() : sizeOfProgram(0) {}
        //calculates the addresses of the functions and basic blocks
        void firstPass(t86::Program &program) {
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
                        auto *jumpIns = dynamic_cast<t86::JumpIns *>(ins);
                        if (jumpIns) {
                            int address = labelAddressMap.at(jumpIns->lbl_->toString());
                            jumpIns->patchLabel(address);
                        }

                        auto *callIns = dynamic_cast<t86::CALLIns *>(ins);
                        if (callIns) {
                            int address = labelAddressMap.at(callIns->lbl_->toString());
                            callIns->patchLabel(address);
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
        size_t sizeOfProgram;
        std::map<std::string, size_t> labelAddressMap;
    };
}