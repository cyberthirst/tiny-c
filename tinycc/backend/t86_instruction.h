//
// Created by Mirek Škrabal on 19.06.2023.
//

#pragma once

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
};

