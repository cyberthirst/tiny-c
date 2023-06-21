//
// Created by Mirek Škrabal on 19.06.2023.
//

#pragma once

#include <memory>

#include "operand.h"

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

    class JMPIns : public Instruction {
    public:
        JMPIns(const LabelOp *lbl)
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
    class name##Ins : public JMPIns { \
    public: \
    name##Ins(const LabelOp *lbl) \
        : JMPIns(lbl) {} \
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

    JMP_INSTRUCTION(JZ);
    JMP_INSTRUCTION(JGE);

    class Program {
    public:
        Program() = default;
        Program(const Program&) = delete;
        Program(Program&&) = default;
        Program& operator=(const Program&) = delete;
        Program& operator=(Program&&) = default;
        ~Program() = default;

        void addInstruction(std::unique_ptr<Instruction> instruction) {
            instructions_.push_back(std::move(instruction));
        }

        std::vector<std::unique_ptr<Instruction>>& instructions() {
            return instructions_;
        }

        const std::vector<std::unique_ptr<Instruction>>& instructions() const {
            return instructions_;
        }

        void print(colors::ColorPrinter & p) const {
            using namespace colors;
            p << COMMENT("; t86 instructions") << NEWLINE;
            p << ".text" << NEWLINE;
            size_t counter = 0;
            for (auto &i : instructions_) {
                p << counter++ << "  " << i->toString() << NEWLINE;
            }
        }


    private:
        std::vector<std::unique_ptr<Instruction>> instructions_;
    };

} // namespace tiny