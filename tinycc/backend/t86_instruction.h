//
// Created by Mirek Škrabal on 19.06.2023.
//

#pragma once


#include <memory>

namespace tiny::t86 {
    class T86Ins {
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
        virtual Opcode opcode() const = 0;
    };

    class UnaryInstruction : public T86Ins {
    protected:
        std::string operand_;
    public:
        UnaryInstruction(const std::string &operand)
                : operand_(operand) {}
    };

    class BinaryInstruction : public T86Ins {
    protected:
        std::string operand1_;
        std::string operand2_;
    public:
        BinaryInstruction(const std::string &operand1, const std::string &operand2)
                : operand1_(operand1), operand2_(operand2) {}
    };

    class JumpInstruction : public T86Ins {
    protected:
        std::string label_;
    public:
        JumpInstruction(const std::string &label)
                : label_(label) {}
    };

    class NoOperandInstruction : public T86Ins {
    };

    // Define concrete classes for each T86Ins

    class PushInstruction : public UnaryInstruction {
    public:
        PushInstruction(const std::string &operand)
                : UnaryInstruction(operand) {}

        Type type() const override {
            return Type::PUSH;
        }
    };

    class PopInstruction : public UnaryInstruction {
    public:
        PopInstruction(const std::string &operand)
                : UnaryInstruction(operand) {}

        Type type() const override {
            return Type::POP;
        }
    };

    class MovInstruction : public BinaryInstruction {
    public:
        MovInstruction(const std::string &operand1, const std::string &operand2)
                : BinaryInstruction(operand1, operand2) {}

        Type type() const override {
            return Type::MOV;
        }
    };

    class CmpInstruction : public BinaryInstruction {
    public:
        CmpInstruction(const std::string &operand1, const std::string &operand2)
                : BinaryInstruction(operand1, operand2) {}

        Type type() const override {
            return Type::CMP;
        }
    };

    class JmpInstruction : public JumpInstruction {
    public:
        JmpInstruction(const std::string &label)
                : JumpInstruction(label) {}

        Type type() const override {
            return Type::JMP;
        }
    };

    class JgeInstruction : public JumpInstruction {
    public:
        JgeInstruction(const std::string &label)
                : JumpInstruction(label) {}

        Type type() const override {
            return Type::JGE;
        }
    };

    class RetInstruction : public NoOperandInstruction {
    public:
        Type type() const override {
            return Type::RET;
        }
    };

    class HaltInstruction : public NoOperandInstruction {
    public:
        Type type() const override {
            return Type::HALT;
        }
    };

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