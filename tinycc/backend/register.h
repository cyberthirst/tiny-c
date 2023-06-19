//
// Created by Mirek Škrabal on 19.06.2023.
//

#pragma once

#include <string>

class Reg {
public:
    enum class Type {
        SP,
        BP
    };
    Reg(Type type, int index) : type_(type), index_(index) { }

    Type type() const { return type_; }
    int index() const { return index_; }

    std::string toString() const {
    }

private:
    Type type_;
    int index_;
};

class Operand {
public:
    virtual ~Operand() = default;
    virtual std::string toString() const = 0;
};

class RegOp : public Operand {
public:
    RegOp(const Reg& reg) : reg_(reg) { }

    std::string toString() const override {
        return reg_.toString();
    }

private:
    Reg reg_;
};

class RegOffsetOp : public Operand {
public:
    RegOffsetOp(const Reg& reg, int offset) : reg_(reg), offset_(offset) { }

    std::string toString() const override {
        return reg_.toString() + " + " + std::to_string(offset_);
    }
private:
    Reg reg_;
    int offset_;
};

class ImmOp : public Operand {
public:
    ImmOp(int value) : value_(value) { }

    std::string toString() const override {
        return std::to_string(value_);
    }

private:
    int value_;
};

