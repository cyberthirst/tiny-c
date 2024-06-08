//
// Created by Mirek Škrabal on 18.06.2023.
//

#pragma once

#include <queue>

#include "../optimizer/il.h"
#include "t86_instruction.h"
#include "program_structures.h"
#include "common/colors.h"
#include "common/symbol.h"
#include "stack.h"
#include "constants.h"
#include "register_alloc.h"

namespace tiny {
    /*
    * A visitor that translates the program in IR to T86.
    * It outputs a Program object that consists of T86 instruction class instances, it doesn't
    * output the assembly.
    */
    class T86CodeGen : public il::IRVisitor {
    public:
        static t86::Program translateProgram(il::Program const &program) {
            T86CodeGen gen{program};
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

        T86CodeGen(const il::Program &program) : ilp_{program} {}
    private:
        t86::Instruction* translate(il::Instruction *child) {
            // need to check if child is not null before dereferencing
            if(child != nullptr){
                visitChild(child);
            }
            return lastResult_;
        }

        void generate(il::Program const &program) {
            Symbol main = Symbol{"main"};
            assert(program.getFunction(main) != nullptr && "main function not found");
            addFunToWorklist(main);
            while (!funWorklist_.empty()) {
                Symbol sfun = funWorklist_.front();
                funWorklist_.pop();
                enterFunction(sfun);
                while (!bbWorklist_.empty()) {
                    il::BasicBlock *bb = bbWorklist_.front();
                    bb_ = f_->addBasicBlock(bb->name);
                    bbWorklist_.pop();
                    for (auto const &instr: bb->getInstructions()) {
                        translate(instr.get());
                    }
                }
                leaveFunction();
            }
        }

        void addBBToWorklist(il::BasicBlock *bb) {
            if (bbVisited_.find(bb) == bbVisited_.end()) {
                bbVisited_.insert(bb);
                bbWorklist_.push(bb);
            }
        }

        void addFunToWorklist(Symbol sym) {
            if (funVisited_.find(sym) == funVisited_.end()) {
                funVisited_.insert(sym);
                funWorklist_.push(sym);
            }
        }

        void generateCdeclPrologue(std::string const &target) {
            bb_ = f_->addBasicBlock("prologue");
            // 1. save base pointer
            (*this) += new t86::PUSHIns(new t86::RegOp(regAllocator_.getBP()));
            // 2. set base pointer to stack pointer
            (*this) += new t86::MOVIns(new t86::RegOp(regAllocator_.getBP()),
                                                     new t86::RegOp(regAllocator_.getSP()));
            // 3. allocate stack space for local variables
            int stackSize = ilf_->getStackSize(true);
            (*this) += new t86::SUBIns(new t86::RegOp(regAllocator_.getSP()),
                                                     new t86::ImmOp(stackSize));
            //currently we have a primitive way of handling function arguments
            // - the caller pushes the arguments on the stack
            // - the callee then MOVs the arguments from the stack to the function's stack frame
            // - the callee then uses the arguments from the stack frame
            // - additionally we assume that all the arguments have fixed size
            // - this is all done as part of the function prologue
            for (size_t i = 0; i < ilf_->numArgs(); ++i) {
                auto *instr = dynamic_cast<il::Instruction::ImmI*>(const_cast<il::Instruction *>(ilf_->getArg(i)));
                //we allocate new register for the argument
                //we then move the argument from the stack to the register
                //we add REG_TO_MEM_WORD because we have to skip the return address
                //we add 1 because the arguments are numbered from 0 - so the first argument is at:
                //  ARG0     <- BP + 2
                //  RET ADDR <- BP + 1
                //  OLD BP   <- BP, SP
                addMOV(instr, new t86::RegOp(regAllocator_.allocate()),
                       new t86::MemRegOffsetOp(regAllocator_.getBP(), REG_TO_MEM_WORD + instr->value + 1));
            }
            (*this) += new t86::JMPIns(new t86::LabelOp(target));
        }

        void generateCdeclEpilogue() {
            auto tmp = f_->addBasicBlock("epilogue");
            (*this) += new t86::JMPIns(new t86::LabelOp(tmp->name));
            bb_ = tmp;
            // cleanup the local variables
            int stackSize = ilf_->getStackSize(true);
            (*this) += new t86::ADDIns(new t86::RegOp(regAllocator_.getSP()),
                                                     new t86::ImmOp(stackSize));
            // 1. restore base pointer
            (*this) += new t86::POPIns(new t86::RegOp(regAllocator_.getBP()));
            // 2. return
            (*this) += new t86::RETIns();
            // the caller is responsible for cleaning up the arguments from the stack
        }

        t86::Function * enterFunction(Symbol name) {
            assert(f_ == nullptr && ilf_ == nullptr && "Cannot enter a function while another function is being translated");
            f_ = p_.addFunction(name);
            ilf_ = ilp_.getFunction(name);
            addBBToWorklist(ilp_.getFunction(name)->start());
            generateCdeclPrologue(bbWorklist_.front()->name);
            return f_;
        }

        void leaveFunction() {
            assert(bbWorklist_.empty() && "Not all basic blocks of a function were translated");
            f_ = nullptr;
            ilf_ = nullptr;
            bb_ = nullptr;
        }

        void addMOV(il::Instruction *i, t86::Operand *dest, t86::Operand *src) {
            (*this) += new t86::MOVIns(dest, src);
            t86::RegOp *regOp = dynamic_cast<t86::RegOp*>(dest);
            if(regOp != nullptr) {
                regMap_[i] = regOp;
            }
        }

        void visit(il::Instruction::ImmI* instr) override {
            switch (instr->opcode) {
                case il::Opcode::LDI: {
                    // Direct translation of IR's LDI to x86's MOV instruction.
                    addMOV(instr, new t86::RegOp(regAllocator_.allocate()), new t86::ImmOp(instr->value));
                    break;
                }
                case il::Opcode::ALLOCA: {
                    int offset = stackAllocator_.allocate(instr, instr->value);
                    //we create a new local variable on the stack and initialize it to 0
                    //the alloca instruction is then accompanied by a store instruction,
                    //   - in ast_to_il we have: (*this) += ST(addr, arg);, the alloca represents the addr
                    //which initializes the variable to appropriate value
                    addMOV(instr, new t86::MemRegOffsetOp(regAllocator_.getBP(), offset), new t86::ImmOp(0));
                    //TODO enable this assert
                    //assert(stackAllocator_.getStackSize() <= f_->getStackSize(true));
                    break;
                }

                case il::Opcode::ARG: {
                    /*il::Instruction::ImmI *i = dynamic_cast<il::Instruction::ImmI*>(instr);
                    addMOV(instr, new RegOp(regAllocator_.allocate()),*
                                      new MemRegOffsetOp(regAllocator_.getBP(), REG_TO_MEM_WORD + i->value + 1));*/
                    //handled directly in the prologue
                    NOT_IMPLEMENTED;
                    break;
                }

                default: {
                    NOT_IMPLEMENTED;
                }
            }
        }

        void visit(il::Instruction::Reg* instr) override {
            switch (instr->opcode) {
                case il::Opcode::LD: {
                    addMOV(instr, new t86::RegOp(regAllocator_.allocate()),
                                  new t86::MemRegOffsetOp(regAllocator_.getBP(), stackAllocator_.getOffset(instr->reg)));
                    break;
                }
                default:
                    NOT_IMPLEMENTED;
            }
        }

        // Visitor for bnary operation on two registers.
        void visit(il::Instruction::RegReg* instr) override {
            switch (instr->opcode) {
                //if the source operand is LD, then we:
                // 1. allocate a new reg
                // 2. move the source operand to the new reg
                // 3. perform the operation on the new reg
                // - this is good because we won't overwrite the variable register
                #define ARITHMETIC_INS(IR_INSTR, T86_INSTR) \
                    case il::Opcode::IR_INSTR: {            \
                    t86::RegOp *op1 = regMap_[instr->reg1]; \
                    t86::RegOp *op2 = regMap_[instr->reg2]; \
                    (*this) += new t86::T86_INSTR##Ins( \
                        op1, \
                        op2 \
                    ); \
                    regMap_[instr] = op1; \
                    break; \
                }
                /*case il::Opcode::ADD: {
                    RegOp *op1 = regMap_[instr->reg1];
                    RegOp *op2 = regMap_[instr->reg2];
                    (*this) += new t86::ADDIns(
                            op1,
                            op2
                    );
                    regMap_[instr] = op1;
                    break;
                }*/

                ARITHMETIC_INS(ADD, ADD)
                ARITHMETIC_INS(SUB, SUB)
                ARITHMETIC_INS(MUL, MUL)
                ARITHMETIC_INS(DIV, DIV)

                case il::Opcode::LT: {
                    (*this) += new t86::CMPIns(
                            regMap_[instr->reg1],
                            regMap_[instr->reg2]
                    );
                    break;
                }
                case il::Opcode::ST: {
                    //1. load the address of the variable to be stored to
                    t86::MemRegOffsetOp *dest = new t86::MemRegOffsetOp(regAllocator_.getBP(),
                                                              stackAllocator_.getOffset(instr->reg1));
                    //2. load the register containing the value to be stored
                    t86::RegOp *src = regMap_[instr->reg2];
                    addMOV(instr, dest, src);
                    break;
                }
                default:
                    NOT_IMPLEMENTED;
            }
        }

        void visit(il::Instruction::Terminator* instr) override {
            switch (instr->opcode) {
                case il::Opcode::RET: {
                    generateCdeclEpilogue();
                    break;
                }
                default:
                    ;
                    //NOT_IMPLEMENTED;
            }
        }

        void visit(il::Instruction::TerminatorB* instr) override {
            switch (instr->opcode) {
                case il::Opcode::JMP: {
                    (*this) += new t86::JMPIns(new t86::LabelOp(instr->target->name));
                    addBBToWorklist(instr->target);
                    break;
                }
                default:
                    ;
                    //NOT_IMPLEMENTED;
            }
        }

        void visit(il::Instruction::TerminatorReg* instr) override {
            switch (instr->opcode) {
                case il::Opcode::RETR: {
                    //we move the return value to the eax register
                    addMOV(instr, new t86::RegOp(regAllocator_.getEAX()), regMap_[instr->reg]);
                    generateCdeclEpilogue();
                    break;
                }
                default:
                    ;
                    //NOT_IMPLEMENTED;
            }
        }

        t86::Instruction *selectJmp(il::Opcode op, const std::string &target) {
            switch (op) {
                case il::Opcode::LT: {
                    return new t86::JGEIns(new t86::LabelOp(target));
                    break;
                }
                default:
                    NOT_IMPLEMENTED;
            }
        }

        void visit(il::Instruction::TerminatorRegBB* instr) override {
            switch (instr->opcode) {
                case il::Opcode::BR: {
                    (*this) += selectJmp(instr->reg->opcode, instr->target2->name);
                    //compile the true branch - that will be the fallthrough case
                    addBBToWorklist(instr->target1);
                    addBBToWorklist(instr->target2);
                    break;
                }
                default:
                    ;
                    //NOT_IMPLEMENTED;
            }
        }

        void visit(il::Instruction::RegRegs* instr) override {
            switch (instr->opcode) {
                case il::Opcode::CALL: {
                    //1. push all the arguments to the stack in reverse order
                    for (auto it = instr->regs.rbegin(); it != instr->regs.rend(); ++it) {
                        (*this) += new t86::PUSHIns(regMap_[*it]);
                    }
                    //2. call the function
                    il::Instruction::ImmS *sfun = dynamic_cast<il::Instruction::ImmS *>(instr->reg);
                    assert(sfun && "Currently we only support calls via symbols");
                    (*this) += new t86::CALLIns(
                            new t86::LabelOp(sfun->value.name())
                    );
                    //3. clean up the stack b
                    //TODO we assume constant size of the arguments
                    //this wouldn't work for structs, probably chars etc..
                    (*this) += new t86::ADDIns(
                            new t86::RegOp(regAllocator_.getSP()),
                            new t86::ImmOp(instr->regs.size())
                    );
                    addFunToWorklist(Symbol{sfun->value});
                    //associate the call instruction with the result register
                    regMap_[instr] = new t86::RegOp{regAllocator_.getEAX()};
                    break;
                }
                default:
                    NOT_IMPLEMENTED;
            }
        }

        //maps original IR instructions to the corresponding registers
        std::unordered_map<il::Instruction*, t86::RegOp *> regMap_;
        t86::Instruction *lastResult_;
        t86::AbstractRegAllocator regAllocator_;
        StackAllocator stackAllocator_;

        t86::BasicBlock *bb_ = nullptr;
        //the function object into which we are compiling (will contain the target insns as opposed to ilf_ which is in IR)
        t86::Function *f_ = nullptr;
        //the function we are currently compiling, it is in the intermediate representation
        const il::Function *ilf_ = nullptr;

        std::queue<il::BasicBlock *> bbWorklist_;
        std::unordered_set<il::BasicBlock *> bbVisited_;

        std::queue<Symbol> funWorklist_;
        std::unordered_set<Symbol> funVisited_;

        t86::Program p_;
        const il::Program &ilp_;
    };

}