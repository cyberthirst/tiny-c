//
// Created by Mirek Škrabal on 19.06.2023.
//

#pragma once

#include <string>
#include "register.h"

namespace tiny::t86 {

    class Operand {
    public:
        virtual ~Operand() = default;

        virtual std::string toString() const = 0;
    };

    class RegOp : public Operand {
    public:
        RegOp(const Reg &reg) : reg_(reg) {}

        std::string toString() const override {
            return reg_.toString();
        }

    private:
        Reg reg_;
    };

    class MemRegOffsetOp : public Operand {
    public:
        MemRegOffsetOp(const Reg &reg, int offset) : reg_(reg), offset_(offset) {}

        std::string toString() const override {
            return "[" + reg_.toString() + (offset_ >= 0 ? " + " : " - ") + std::to_string(std::abs(offset_)) + "]";
        }

    private:
        Reg reg_;
        int offset_;
    };

    class RegOffsetOp : public Operand {
    public:
        RegOffsetOp(const Reg &reg, int offset) : reg_(reg), offset_(offset) {}

        std::string toString() const override {
            return reg_.toString() + (offset_ >= 0 ? " + " : " - ") + std::to_string(std::abs(offset_));
        }

    private:
        Reg reg_;
        int offset_;
    };

    class ImmOp : public Operand {
    public:
        ImmOp(int value) : value_(value) {}

        std::string toString() const override {
            return std::to_string(value_);
        }

    private:
        int value_;
    };

    class LabelOp : public Operand {
    public:
        LabelOp(std::string label) : label_(std::move(label)), address_(-1) {}

        void patch(int address) {
            address_ = address;
        }

        std::string toString() const override {
            if (address_ >= 0) { // check if the address has been patched.
                return std::to_string(address_);
            }
            return label_;
        }

    private:
        std::string label_;
        int address_;
    };

}