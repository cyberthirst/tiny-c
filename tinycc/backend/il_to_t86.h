//
// Created by Mirek Škrabal on 18.06.2023.
//

#pragma once

#include <queue>

#include "../optimizer/il.h"
#include "reg_allocator.h"
#include "t86_instruction.h"
#include "common/colors.h"
#include "common/symbol.h"

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
        //
        StackAllocator() : offset_(8) {}

        int allocate(Instruction const *var, size_t size) {
            offsets_.emplace(var, offset_);
            offset_ += size;
            return -offsets_.at(var);
        }

        int getOffset(Instruction const *var) const {
            return -offsets_.at(var);
        }

        int getStackSize() const {
            return offset_ - 8;
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
            return std::move(gen.p_);
        }

        T86CodeGen & operator += (t86::Instruction *ins) {
            //p_.addInstruction(std::unique_ptr<t86::Instruction>{ins});
            bb_->append(ins);
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
            Symbol main = Symbol{"main"};
            enterFunction(main);
            bbWorklist_.emplace(program.getFunction(main)->start());
            generateBasicBlock();
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

        t86::Function * enterFunction(Symbol name) {
            ASSERT(f_ == nullptr);
            f_ = p_.addFunction(name);
            return f_;
        }

        void leaveFunction() {
            f_ = nullptr;
        }

        void generateBasicBlock() {
            while (!bbWorklist_.empty()) {
                BasicBlock *bb = bbWorklist_.front();
                bb_ = f_->addBasicBlock(bb->name);
                bbWorklist_.pop();
                for (auto const &instr: bb->getInstructions()) {
                    translate(instr.get());
                }
            }
        }

        void addMOV(Instruction *i, Operand *dest, Operand *src) {
            (*this) += new t86::MOVIns(dest, src);
            RegOp *regOp = dynamic_cast<RegOp*>(dest);
            if(regOp != nullptr) {
                regMap_[i] = regOp;
            }
        }

        void visit(Instruction::ImmI* instr) override {
            switch (instr->opcode) {
                case Opcode::LDI: {
                    // Direct translation of IR's LDI to x86's MOV instruction.
                    addMOV(instr, new RegOp(regAllocator_.allocate()), new ImmOp(instr->value));
                    break;
                }
                case Opcode::ALLOCA: {
                    int offset = stackAllocator_.allocate(instr, instr->value);
                    //we create a new local variable on the stack and initialize it to 0
                    //the alloca instruction is then accompanied by a store instruction,
                    //   - in ast_to_il we have: (*this) += ST(addr, arg);, the alloca represents the addr
                    //which initializes the variable to appropriate value
                    addMOV(instr, new MemRegOffsetOp(regAllocator_.getBP(), offset), new ImmOp(0));
                    assert(stackAllocator_.getStackSize() <= f_->getStackSize(true));
                    break;
                }
                default: {
                    NOT_IMPLEMENTED;
                }
            }
        }

        void visit(Instruction::Reg* instr) override {
            switch (instr->opcode) {
                case Opcode::LD: {
                    addMOV(instr, new RegOp(regAllocator_.allocate()),
                                  new MemRegOffsetOp(regAllocator_.getBP(), stackAllocator_.getOffset(instr->reg)));
                    break;
                }
                default:
                    NOT_IMPLEMENTED;
            }
        }

        // Visitor for bnary operation on two registers.
        void visit(Instruction::RegReg* instr) override {
            switch (instr->opcode) {
                case Opcode::ADD: {
                    RegOp *op1 = regMap_[instr->reg1];
                    RegOp *op2 = regMap_[instr->reg2];
                    (*this) += new t86::ADDIns(
                            op1,
                            op2
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
                case Opcode::ST: {
                    //1. load the address of the variable to be stored to
                    MemRegOffsetOp *dest = new MemRegOffsetOp(regAllocator_.getBP(),
                                                              stackAllocator_.getOffset(instr->reg1));
                    //2. load the register containing the value to be stored
                    RegOp *src = regMap_[instr->reg2];
                    addMOV(instr, dest, src);
                    break;
                }
                default:
                    NOT_IMPLEMENTED;
            }
        }

        void visit(Instruction::Terminator* instr) override {
            switch (instr->opcode) {
                case Opcode::RET: {
                    break;
                }
                default:
                    ;
                    //NOT_IMPLEMENTED;
            }
        }

        void visit(Instruction::TerminatorB* instr) override {
            switch (instr->opcode) {
                case Opcode::JMP: {
                    (*this) += new t86::JMPIns(new LabelOp(bb_->name));
                    break;
                }
                default:
                    ;
                    //NOT_IMPLEMENTED;
            }
        }

        void visit(Instruction::TerminatorReg* instr) override {
            switch (instr->opcode) {
                case Opcode::RETR: {

                    break;
                }
                default:
                    ;
                    //NOT_IMPLEMENTED;
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

        //maps original IR instructions to the corresponding registers
        std::unordered_map<Instruction*, RegOp *> regMap_;
        t86::Instruction *lastResult_;
        AbstractRegAllocator regAllocator_;
        StackAllocator stackAllocator_;

        t86::BasicBlock *bb_;
        t86::Function *f_;

        std::queue<BasicBlock *> bbWorklist_;

        t86::Program p_;
    };

}