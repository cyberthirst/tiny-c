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
        virtual bool equals(const Operand *other) const = 0;
        virtual std::size_t hash() const = 0;
    };

    class RegOp : public Operand {
    public:
        RegOp(const Reg &reg) : reg_(reg) {}

        std::string toString() const override {
            return reg_.toString();
        }


        bool equals(const Operand *other) const override {
            auto otherRegOp = dynamic_cast<const RegOp*>(other);
            return otherRegOp != nullptr && reg_ == otherRegOp->reg_;
        }

        std::size_t hash() const override {
            return std::hash<int>{}(static_cast<int>(reg_.type())) ^ std::hash<int>{}(reg_.index());
        }
        Reg reg_;
    };

    class MemRegOffsetOp : public Operand {
    public:
        MemRegOffsetOp(const Reg &reg, int offset) : reg_(reg), offset_(offset) {}

        std::string toString() const override {
            return "[" + reg_.toString() + (offset_ >= 0 ? " + " : " - ") + std::to_string(std::abs(offset_)) + "]";
        }

        bool equals(const Operand *other) const override {
            auto otherMemRegOffsetOp = dynamic_cast<const MemRegOffsetOp*>(other);
            return otherMemRegOffsetOp != nullptr && reg_ == otherMemRegOffsetOp->reg_ && offset_ == otherMemRegOffsetOp->offset_;
        }

        std::size_t hash() const override {
            return std::hash<int>{}(static_cast<int>(reg_.type())) ^ std::hash<int>{}(reg_.index()) ^ std::hash<int>{}(offset_);
        }

        Reg reg_;
        int offset_;
    };

    class RegOffsetOp : public Operand {
    public:
        RegOffsetOp(const Reg &reg, int offset) : reg_(reg), offset_(offset) {}

        std::string toString() const override {
            return reg_.toString() + (offset_ >= 0 ? " + " : " - ") + std::to_string(std::abs(offset_));
        }

        bool equals(const Operand *other) const override {
            auto otherRegOffsetOp = dynamic_cast<const RegOffsetOp*>(other);
            return otherRegOffsetOp != nullptr && reg_ == otherRegOffsetOp->reg_ && offset_ == otherRegOffsetOp->offset_;
        }

        std::size_t hash() const override {
            return std::hash<int>{}(static_cast<int>(reg_.type())) ^ std::hash<int>{}(reg_.index()) ^ std::hash<int>{}(offset_);
        }

        Reg reg_;
        int offset_;
    };

    class ImmOp : public Operand {
    public:
        ImmOp(int value) : value_(value) {}

        std::string toString() const override {
            return std::to_string(value_);
        }

        bool equals(const Operand *other) const override {
            auto otherImmOp = dynamic_cast<const ImmOp*>(other);
            return otherImmOp != nullptr && value_ == otherImmOp->value_;
        }

        std::size_t hash() const override {
            return std::hash<int>{}(value_);
        }

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

        bool equals(const Operand *other) const override {
            auto otherLabelOp = dynamic_cast<const LabelOp*>(other);
            return otherLabelOp != nullptr && label_ == otherLabelOp->label_ && address_ == otherLabelOp->address_;
        }

        std::size_t hash() const override {
            return std::hash<std::string>{}(label_) ^ std::hash<int>{}(address_);
        }

        std::string label_;
        int address_;
    };


}