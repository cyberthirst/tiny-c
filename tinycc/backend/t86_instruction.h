//
// Created by Mirek Škrabal on 19.06.2023.
//

#pragma once

#include <memory>

#include "operand.h"
#include "common/colors.h"

namespace tiny::t86 {
    class Instruction {
    public:
        //opcodes from: https://github.com/Gregofi/t86-with-debug/blob/master/src/t86/instruction.h
        enum class Opcode {
            MOV,
            LEA,
            NOP,
            HALT,
            ADD,
            SUB,
            INC,
            DEC,
            MUL,
            DIV,
            MOD,
            IMUL,
            IDIV,
            CMP,
            JMP,
            JZ,
            JNZ,
            JE,
            JNE,
            JG,
            JGE,
            JL,
            JLE,
            JA,
            JAE,
            CALL,
            RET,
            PUSH,
            POP,
        };
        virtual ~Instruction() = default;
        virtual std::string toString() const = 0;
    };



    class UnaryIns : public Instruction {
    public:
        UnaryIns(const Operand *operand)
                : operand_(operand) {}
        const Operand *operand_;
    };

    class BinaryIns : public Instruction {
    public:
        BinaryIns(const Operand *operand1, const Operand *operand2)
                : operand1_(operand1), operand2_(operand2) {}
        const Operand *operand1_;
        const Operand *operand2_;
    };

    class NoOpIns : public Instruction {
    };

    class JumpIns : public Instruction {
    public:
        JumpIns(const LabelOp *lbl)
                : lbl_(lbl) {}
        const LabelOp *lbl_;
    };


    #define UNARY_INSTRUCTION(name) \
    class name##Ins : public UnaryIns { \
    public: \
    name##Ins(const Operand *operand) \
            : UnaryIns(operand) {} \
    std::string toString() const override { \
        return #name " " + operand_->toString(); \
    } \
    };

    #define BINARY_INSTRUCTION(name) \
    class name##Ins : public BinaryIns { \
    public: \
    name##Ins(const Operand *dest, const Operand *src) \
            : BinaryIns(dest, src) {} \
    std::string toString() const override { \
        return #name " " + operand1_->toString() + ", " + operand2_->toString(); \
    } \
    };

    #define NOOP_INSTRUCTION(name) \
    class name##Ins : public NoOpIns { \
    public: \
    std::string toString() const override { \
        return #name; \
    } \
    };

    #define JMP_INSTRUCTION(name) \
    class name##Ins : public JumpIns { \
    public: \
    name##Ins(const LabelOp *lbl) \
        : JumpIns(lbl) {} \
    std::string toString() const override { \
        return #name " " + lbl_->toString(); \
    } \
    };

    UNARY_INSTRUCTION(PUSH);
    UNARY_INSTRUCTION(POP);

    BINARY_INSTRUCTION(MOV);
    BINARY_INSTRUCTION(CMP);
    BINARY_INSTRUCTION(SUB);
    BINARY_INSTRUCTION(ADD);

    NOOP_INSTRUCTION(RET);
    NOOP_INSTRUCTION(HALT);

    JMP_INSTRUCTION(JMP);
    JMP_INSTRUCTION(JZ);
    JMP_INSTRUCTION(JGE);

    class BasicBlock {
    public:

        std::string const name;

        BasicBlock():
                name{makeUniqueName()} {
        }

        BasicBlock(std::string const & name):
                name{makeUniqueName(name)} {
        }

        /*bool terminated() const {
            if (insns_.empty())
                return false;
            return dynamic_cast<Instruction::Terminator*>(insns_.back().get()) != nullptr;
        }*/

        /** Appends the instruction to the given basic block.
         */
        Instruction * append(Instruction * ins) {
            insns_.push_back(std::unique_ptr<Instruction>{ins});
            return ins;
        }

        size_t size() const { return insns_.size(); }

        Instruction * operator[](size_t i) const { return insns_[i].get(); }

        const std::vector<std::unique_ptr<Instruction>>& getInstructions() const {
            return insns_;
        }

    private:

        friend class Function;
        friend class Program;

        void print(colors::ColorPrinter & p) const {
            using namespace colors;
            p << IDENT(name) << SYMBOL(":") << INDENT;
            for (auto & i : insns_) {
                p << NEWLINE;
                //TODO print the instruction
                //i->print(p);
            }
            p << DEDENT;
        }


        static std::string makeUniqueName() {
            return STR("bb" << nextUniqueId());
        }

        static std::string makeUniqueName(std::string const & from) {
            return STR(from << nextUniqueId());
        }

        static size_t nextUniqueId() {
            static size_t i = 0;
            return i++;
        }

        std::vector<std::unique_ptr<Instruction>> insns_;
    };


    class Function {
    public:
        BasicBlock * addBasicBlock() {
            std::unique_ptr<BasicBlock> bb{new BasicBlock{}};
            bbs_.push_back(std::move(bb));
            return bbs_.back().get();
        }

        BasicBlock * addBasicBlock(std::string const & name) {
            std::unique_ptr<BasicBlock> bb{new BasicBlock{name}};
            bbs_.push_back(std::move(bb));
            return bbs_.back().get();
        }


        Instruction * addArg(Instruction * arg) {
            std::unique_ptr<Instruction> a{arg};
            args_.push_back(std::move(a));
            return arg;
        }

        size_t numArgs() const { return args_.size(); }

        //Instruction const * getArg(size_t i) const { return args_[i].get(); }

        const std::vector<std::unique_ptr<BasicBlock>>& getBasicBlocks() const { return bbs_; }

        std::vector<std::unique_ptr<BasicBlock>>& getBasicBlocks() { return bbs_; }

        void print(colors::ColorPrinter & p) const {
            using namespace colors;
            if (! args_.empty()) {
                p << NEWLINE << COMMENT("; arguments ") << INDENT;
                for (auto & arg : args_) {
                    p << NEWLINE;
                    //arg->print(p);
                }
                p << DEDENT;
            }
            p << NEWLINE << COMMENT("; number of basic blocks: ") << bbs_.size();
            for (auto & bb : bbs_) {
                p << NEWLINE;
                bb->print(p);
            }
        }

        BasicBlock * start() const { return bbs_[0].get(); }
        //RegType retType_;
    private:
        std::vector<std::unique_ptr<Instruction>> args_;
        std::vector<std::unique_ptr<BasicBlock>> bbs_;
    };

    class Program {
    public:
        Program() = default;
        Program(const Program&) = delete;
        Program(Program&&) = default;
        Program& operator=(const Program&) = delete;
        Program& operator=(Program&&) = default;
        ~Program() = default;

        /*void addInstruction(std::unique_ptr<Instruction> instruction) {
            instructions_.push_back(std::move(instruction));
        }

        std::vector<std::unique_ptr<Instruction>>& instructions() {
            return instructions_;
        }


        const std::vector<std::unique_ptr<Instruction>>& instructions() const {
            return instructions_;
        }

        size_t size() const {
            return instructions_.size();
        }*/
        Function * addFunction(Symbol name){
            if (functions_.find(name) != functions_.end()) {
                throw std::runtime_error(STR("function " << name << " already exists"));
            }
            Function * f = new Function{};
            functions_.insert(std::make_pair(name, f));
            return f;
        }


        void print(colors::ColorPrinter & p) const {
            using namespace colors;
            p << COMMENT("; t86 instructions") << NEWLINE;
            p << ".text" << NEWLINE;
            size_t counter = 0;
            /*for (auto &i : instructions_) {
                p << counter++ << "  " << i->toString() << NEWLINE;
            }*/
        }


    private:
        //std::vector<std::unique_ptr<Instruction>> instructions_;
        std::unordered_map<Symbol, Function *> functions_;
    };

} // namespace tiny