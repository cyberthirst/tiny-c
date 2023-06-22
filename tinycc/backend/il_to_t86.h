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

namespace tiny {
    /*
    * A visitor that translates the program in IR to T86.
    * It outputs a Program object that consists of T86 instruction class instances, it doesn't
    * output the assembly.
    */
    class T86CodeGen : public il::IRVisitor {
    public:
        static t86::Program translateProgram(il::Program const &program) {
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
        t86::Instruction* translate(il::Instruction *child) {
            // need to check if child is not null before dereferencing
            if(child != nullptr){
                visitChild(child);
            }
            return lastResult_;
        }

        void generate(il::Program const &program) {
            Symbol main = Symbol{"main"};
            enterFunction(main);
            addBBToWorklist(program.getFunction(main)->start());
            generateBasicBlock();
        }

        void addBBToWorklist(il::BasicBlock *bb) {
            if (bbVisited_.find(bb) == bbVisited_.end()) {
                bbVisited_.insert(bb);
                bbWorklist_.push(bb);
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
                il::BasicBlock *bb = bbWorklist_.front();
                bb_ = f_->addBasicBlock(bb->name);
                bbWorklist_.pop();
                for (auto const &instr: bb->getInstructions()) {
                    translate(instr.get());
                }
                std::cout << colors::ColorPrinter::colorize(p_);
            }
        }

        void addMOV(il::Instruction *i, Operand *dest, Operand *src) {
            (*this) += new t86::MOVIns(dest, src);
            RegOp *regOp = dynamic_cast<RegOp*>(dest);
            if(regOp != nullptr) {
                regMap_[i] = regOp;
            }
        }

        void visit(il::Instruction::ImmI* instr) override {
            switch (instr->opcode) {
                case il::Opcode::LDI: {
                    // Direct translation of IR's LDI to x86's MOV instruction.
                    addMOV(instr, new RegOp(regAllocator_.allocate()), new ImmOp(instr->value));
                    break;
                }
                case il::Opcode::ALLOCA: {
                    int offset = stackAllocator_.allocate(instr, instr->value);
                    //we create a new local variable on the stack and initialize it to 0
                    //the alloca instruction is then accompanied by a store instruction,
                    //   - in ast_to_il we have: (*this) += ST(addr, arg);, the alloca represents the addr
                    //which initializes the variable to appropriate value
                    addMOV(instr, new MemRegOffsetOp(regAllocator_.getBP(), offset), new ImmOp(0));
                    //TODO enable this assert
                    //assert(stackAllocator_.getStackSize() <= f_->getStackSize(true));
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
                    addMOV(instr, new RegOp(regAllocator_.allocate()),
                                  new MemRegOffsetOp(regAllocator_.getBP(), stackAllocator_.getOffset(instr->reg)));
                    break;
                }
                default:
                    NOT_IMPLEMENTED;
            }
        }

        // Visitor for bnary operation on two registers.
        void visit(il::Instruction::RegReg* instr) override {
            switch (instr->opcode) {
                case il::Opcode::ADD: {
                    RegOp *op1 = regMap_[instr->reg1];
                    RegOp *op2 = regMap_[instr->reg2];
                    (*this) += new t86::ADDIns(
                            op1,
                            op2
                    );
                    regMap_[instr] = op1;
                    break;
                }

                case il::Opcode::SUB: {
                    // Direct translation of IR's SUB to x86's SUB instruction.
                    /**this += new t86::SUBIns(
                            new RegOp(instr->dst),
                            new RegOp(instr->src)
                    );*/
                    break;
                }

                case il::Opcode::LT: {
                    (*this) += new t86::CMPIns(
                            regMap_[instr->reg1],
                            regMap_[instr->reg2]
                    );
                    break;
                }
                case il::Opcode::ST: {
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

        void visit(il::Instruction::Terminator* instr) override {
            switch (instr->opcode) {
                case il::Opcode::RET: {
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
                    (*this) += new t86::JMPIns(new LabelOp(bb_->name));
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
                    return new t86::JGEIns(new LabelOp(target));
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
        std::unordered_map<il::Instruction*, RegOp *> regMap_;
        t86::Instruction *lastResult_;
        AbstractRegAllocator regAllocator_;
        StackAllocator stackAllocator_;

        t86::BasicBlock *bb_ = nullptr;
        t86::Function *f_ = nullptr;

        std::queue<il::BasicBlock *> bbWorklist_;
        std::unordered_set<il::BasicBlock *> bbVisited_;

        t86::Program p_;
    };

}