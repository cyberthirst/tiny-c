//
// Created by Mirek Škrabal on 18.06.2023.
//

#pragma once

#include "../optimizer/il.h"
#include "reg_allocator.h"
#include "t86_instruction.h"

namespace tiny {
    /*
     * A stack allocator that allocates variables on the stack.
     * It is used to allocate local variables in functions, keeps
     * track of the offsets of the variables in the stack frame.
     *
     * Currently, a very stupid strategy is used. Each local var
     * is allocated on the stack, we ignore scoping, so if a variable
     * is out of scope, we don't reuse it's space. This is a tradeoff,
     * since if we wanted an effective strategy, we would need liveness
     * and points-to analyses.
     */
    class StackAllocator {
    public:
        StackAllocator() : offset_(0) {}

        int allocate(Instruction const *var, size_t size) {
            offsets_.emplace(var, offset_);
            offset_ += size;
            return offsets_.at(var);
        }

        int getOffset(Instruction const *var) const {
            return offsets_.at(var);
        }

        int getStackSize() const {
            return offset_;
        }
    private:
        int offset_;
        std::unordered_map<Instruction const *, int> offsets_;
    };


    /*
    * A visitor that translates the program in IR to T86.
    * It outputs a Program object that consists of T86 instruction class instances, it doesn't
    * output the assembly.
    */
    class T86CodeGen : public IRVisitor {
    public:
        static t86::Program translateProgram(Program const &program) {
            T86CodeGen gen;
            gen.generate(program);
            //TODO generate HALT instruction
            return std::move(gen.program_);
        }

        T86CodeGen & operator += (t86::Instruction *ins) {
            program_.addInstruction(std::unique_ptr<t86::Instruction>{ins});
            lastResult_ = ins;
            return *this;
        }
    private:
        t86::Instruction* translate(Instruction *child) {
            // need to check if child is not null before dereferencing
            if(child != nullptr){
                visitChild(child);
            }
            return lastResult_;
        }

        void generate(Program const &program) {
            for (auto const &[name, function]: program.getFunctions()) {
                currentFunction_ = function;
                generateFunction(*function);
            }
        }

        void generateCdeclPrologue(size_t stackSize) {
            // 1. save base pointer
            (*this) += new t86::PUSHIns(new RegOp(regAllocator_.getBP()));
            // 2. set base pointer to stack pointer
            (*this) += new t86::MOVIns(new RegOp(regAllocator_.getBP()),
                                                     new RegOp(regAllocator_.getSP()));
            // 3. allocate stack space for local variables
            (*this) += new t86::SUBIns(new RegOp(regAllocator_.getSP()),
                                                     new ImmOp(stackSize));
        }

        void generateCdeclEpilogue() {
            // 1. restore base pointer
            (*this) += new t86::POPIns(new RegOp(regAllocator_.getBP()));
            // 2. return
            (*this) += new t86::RETIns();
        }

        void generateFunction(Function const &function) {
            generateCdeclPrologue(function.getStackSize(true));
            for (auto const &basicBlock: function.getBasicBlocks()) {
                generateBasicBlock(*basicBlock);
            }
            generateCdeclEpilogue();
        }

        void generateBasicBlock(BasicBlock const &basicBlock) {
            for (auto &ins : basicBlock.getInstructions()) {
                translate(ins.get());
            }
        }

        void visit(Instruction::ImmI* instr) override {
            switch (instr->opcode) {
                case Opcode::LDI: {
                    // Direct translation of IR's LDI to x86's MOV instruction.
                    *this += new t86::MOVIns(
                            new RegOp(regAllocator_.allocate()),
                            new ImmOp(instr->value)
                    );
                    break;
                }
                case Opcode::ALLOCA: {
                    int offset = stackAllocator_.allocate(instr, instr->value);
                    //we create a new local variable on the stack and initialize it to 0
                    //the alloca instruction is then accompanied by a store instruction,
                    //   - in ast_to_il we have: (*this) += ST(addr, arg);, the alloca represents the addr
                    //which initializes the variable to appropriate value
                    (*this) += new t86::MOVIns(
                            new RegOffsetOp(regAllocator_.getBP(), offset),
                            new ImmOp(0)
                            );
                    assert(stackAllocator_.getStackSize() <= currentFunction_->getStackSize(true));
                    break;
                }
                default: {
                    NOT_IMPLEMENTED;
                }
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
        }*/

        // Visitor for binary operation on two registers.
        void visit(Instruction::RegReg* instr) override {
            switch (instr->opcode) {
                case Opcode::ADD: {
                    t86::MOVIns *i1 = dynamic_cast<t86::MOVIns*>(translate(instr->reg1));
                    t86::MOVIns *i2 = dynamic_cast<t86::MOVIns*>(translate(instr->reg2));
                    assert(i1 != nullptr && i2 != nullptr && "ADD instruction should be preceded by two MOV instructions");
                    (*this) += new t86::ADDIns(
                        i1->operand1_,
                        i2->operand1_
                    );
                    break;
                }

                case Opcode::SUB: {
                    // Direct translation of IR's SUB to x86's SUB instruction.
                    /**this += new t86::SUBIns(
                            new RegOp(instr->dst),
                            new RegOp(instr->src)
                    );*/
                    break;
                }
                default:
                    NOT_IMPLEMENTED;
            }
        }
        /*
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
        t86::Instruction *lastResult_;
        AbstractRegAllocator regAllocator_;
        StackAllocator stackAllocator_;
        Function *currentFunction_;
        t86::Program program_;
    };

}