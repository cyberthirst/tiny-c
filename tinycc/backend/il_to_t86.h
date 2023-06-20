//
// Created by Mirek Škrabal on 18.06.2023.
//

#pragma once

#include "../optimizer/il.h"
#include "reg_allocator.h"
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
            //TODO generate HALT instruction
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

        void generateCdeclPrologue(size_t stackSize) {
            // 1. save base pointer
            (*this) += std::make_unique<t86::PUSHIns>(new RegOp(allocator_.getBP()));
            // 2. set base pointer to stack pointer
            (*this) += std::make_unique<t86::MOVIns>(new RegOp(allocator_.getBP()),
                                                     new RegOp(allocator_.getSP()));
            // 3. allocate stack space for local variables
            (*this) += std::make_unique<t86::SUBIns>(new RegOp(allocator_.getSP()),
                                                     new ImmOp(stackSize));
        }

        void generateCdeclEpilogue() {
            // 1. restore base pointer
            (*this) += std::make_unique<t86::POPIns>(new RegOp(allocator_.getBP()));
            // 2. return
            (*this) += std::make_unique<t86::RETIns>();
        }

        void generateFunction(Function const &function) {
            generateCdeclPrologue(function.getStackSize());
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

        void visit(Instruction::ImmI* instr) override {
            switch (instr->opcode) {
                case Opcode::LDI:
                    // Direct translation of IR's LDI to x86's MOV instruction.
                    *this += std::make_unique<t86::MOVIns>(
                            new RegOp(allocator_.allocate()),
                            new ImmOp(instr->value)
                    );
                    break;
                case Opcode::ALLOCA:
                default:
                    NOT_IMPLEMENTED;
            }
        }

        // Visitor for register load instruction.
        /*void visit(Instruction::Reg* instr) override {
            switch (instr->opcode) {
                case Opcode::LD:
                    // Direct translation of IR's LD to x86's MOV instruction.
                    // Assumes instr->dst is the destination register and instr->src is the source register.
                    *this += std::make_unique<t86::MOVIns>(
                            new RegOp(instr->dst),
                            new RegOp(instr->src)
                    );
                    break;

                    // Add more cases as needed...
                default:
                    NOT_IMPLEMENTED;
            }
        }

        // Visitor for binary operation on two registers.
        void visit(Instruction::RegReg* instr) override {
            switch (instr->opcode) {
                case Opcode::ADD:
                    // Direct translation of IR's ADD to x86's ADD instruction.
                    // Assumes instr->dst is the destination register and instr->src is the source register.
                    *this += std::make_unique<t86::ADDIns>(
                            new RegOp(instr->dst),
                            new RegOp(instr->src)
                    );
                    break;

                case Opcode::SUB:
                    // Direct translation of IR's SUB to x86's SUB instruction.
                    *this += std::make_unique<t86::SUBIns>(
                            new RegOp(instr->dst),
                            new RegOp(instr->src)
                    );
                    break;

                    // Add more cases as needed...
                default:
                    NOT_IMPLEMENTED;
            }
        }

        // Visitor for function call.
        void visit(Instruction::RegRegs* instr) override {
            switch (instr->opcode) {
                case Opcode::CALL:
                    // Translation of IR's CALL to x86's CALL instruction.
                    // This assumes that instr->funName is the name of the function to call.
                    *this += std::make_unique<t86::CALLIns>(
                            new LabelOp(instr->funName)
                    );
                    break;

                    // Add more cases as needed...
                default:
                    NOT_IMPLEMENTED;
            }
        }*/
        AbstractRegAllocator allocator_;
        t86::Program program_;
    };

}