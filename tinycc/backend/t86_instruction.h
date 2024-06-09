//
// Created by Mirek Škrabal on 19.06.2023.
//

#pragma once

#include <memory>

#include "operand.h"

namespace tiny::t86 {
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

    class Instruction {
    public:
        virtual ~Instruction() = default;
        virtual std::string toString() const = 0;

        virtual std::vector<Operand*> getOperands() = 0;
    };

    class UnaryIns : public Instruction {
    public:
        UnaryIns(Operand *operand)
                : operand_(operand) {}

        Operand *operand_;

        std::vector<Operand*> getOperands() override {
            return {operand_};
        }
    };

    class BinaryIns : public Instruction {
    public:
        BinaryIns(Operand *operand1, Operand *operand2)
                : operand1_(operand1), operand2_(operand2) {}

        Operand *operand1_;
        Operand *operand2_;

        std::vector<Operand*> getOperands() override {
            return {operand1_, operand2_};
        }
    };

    class NoOpIns : public Instruction {
    public:
        std::vector<Operand*> getOperands() override {
            return {};  // No operands
        }
    };

    class LblIns : public Instruction {
    public:
        LblIns(LabelOp *lbl)
                : lbl_(lbl) {}

        void patchLabel(int address) {
            lbl_->patch(address);
        }
        LabelOp *lbl_;

        std::vector<Operand*> getOperands() override {
            return {lbl_};
        }
    };

    class JumpIns : public LblIns {
    public:
        JumpIns(LabelOp *lbl)
                : LblIns(lbl) {}

    };

    class CALLIns : public LblIns {
    public:
        CALLIns(LabelOp *lbl)
                : LblIns(lbl) {}

        std::string toString() const override {
            return "CALL " + lbl_->toString();
        }
    };

    #define UNARY_INSTRUCTION(name) \
    class name##Ins : public UnaryIns { \
    public: \
    name##Ins(Operand *operand) \
            : UnaryIns(operand) {} \
    std::string toString() const override { \
        return #name " " + operand_->toString(); \
    } \
    };

    #define BINARY_INSTRUCTION(name) \
    class name##Ins : public BinaryIns { \
    public: \
    name##Ins(Operand *dest, Operand *src) \
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
    name##Ins(LabelOp *lbl) \
        : JumpIns(lbl) {} \
    std::string toString() const override { \
        return #name " " + lbl_->toString(); \
    } \
    };

    UNARY_INSTRUCTION(PUSH);
    UNARY_INSTRUCTION(POP);
    UNARY_INSTRUCTION(PUTNUM);

    BINARY_INSTRUCTION(MOV);
    BINARY_INSTRUCTION(CMP);
    BINARY_INSTRUCTION(SUB);
    BINARY_INSTRUCTION(ADD);
    BINARY_INSTRUCTION(MUL);
    BINARY_INSTRUCTION(DIV);

    NOOP_INSTRUCTION(RET);
    NOOP_INSTRUCTION(HALT);
    NOOP_INSTRUCTION(NOP);

    JMP_INSTRUCTION(JMP);
    JMP_INSTRUCTION(JZ);
    JMP_INSTRUCTION(JGE);

} // namespace tiny