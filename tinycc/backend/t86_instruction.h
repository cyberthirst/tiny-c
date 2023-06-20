//
// Created by Mirek Škrabal on 19.06.2023.
//

#pragma once

#include <memory>

#include "operand.h"

namespace tiny::t86 {
    class T86Ins {
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
    };



    class UnaryIns : public T86Ins {
    public:
        UnaryIns(const Operand *operand)
                : operand_(operand) {}
    protected:
        const Operand *operand_;
    };

    class BinaryIns : public T86Ins {
    public:
        BinaryIns(const Operand *operand1, const Operand *operand2)
                : operand1_(operand1), operand2_(operand2) {}
    protected:
        const Operand *operand1_;
        const Operand *operand2_;
    };

    class NoOpIns : public T86Ins {
    };

    class JMPIns : public T86Ins {
    public:
        JMPIns(const LabelOp *lbl)
                : lbl_(lbl) {}
    protected:
        const LabelOp *lbl_;
    };


    #define UNARY_INSTRUCTION(name) \
    class name##Ins : public UnaryIns { \
    public: \
        name##Ins(const Operand *operand) \
                : UnaryIns(operand) {} \
    };

    #define BINARY_INSTRUCTION(name) \
    class name##Ins : public BinaryIns { \
    public: \
        name##Ins(const Operand *dest, const Operand *src) \
                : BinaryIns(dest, src) {} \
    };

    #define NOOP_INSTRUCTION(name) \
    class name##Ins : public NoOpIns { \
    public: \
    };

    #define JMP_INSTRUCTION(name) \
    class name##Ins : public JMPIns { \
    public: \
        name##Ins(const LabelOp *lbl) \
            : JMPIns(lbl) {} \
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

        void addInstruction(std::unique_ptr<T86Ins> instruction) {
            instructions_.push_back(std::move(instruction));
        }

        std::vector<std::unique_ptr<T86Ins>>& instructions() {
            return instructions_;
        }

        const std::vector<std::unique_ptr<T86Ins>>& instructions() const {
            return instructions_;
        }

    private:
        std::vector<std::unique_ptr<T86Ins>> instructions_;
    };

} // namespace tiny