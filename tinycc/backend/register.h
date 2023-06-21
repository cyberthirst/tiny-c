//
// Created by Mirek Škrabal on 19.06.2023.
//

#pragma once

#include <string>
#include <climits> // for INT_MAX

namespace tiny {

    class Reg {
    public:
        enum class Type {
            SP,
            BP,
            GP,
        };

        Reg(Type type, int index) : type_(type), index_(index) {}

        Type type() const { return type_; }

        int index() const { return index_; }

        std::string toString() const {
            switch (type_) {
                case Type::SP:
                    return "SP";
                case Type::BP:
                    return "BP";
                case Type::GP:
                    return "R" + std::to_string(index_);
                default:
                    return "Invalid";
            }
        }

    private:
        Type type_;
        int index_;
    };

    class RegAllocator {
    public:
        RegAllocator()
                : SP(Reg::Type::SP, INT_MAX),
                  BP(Reg::Type::BP, INT_MAX - 1) {
        }

        virtual Reg allocate() = 0;  // Pure virtual function

        const Reg &getSP() const { return SP; }

        const Reg &getBP() const { return BP; }

    protected:
        Reg SP;  // Stack Pointer register
        Reg BP;  // Base Pointer register
    };


/*
 * Allocates abstract registers, ie ignores the limit set by the architecture.
 * Used during the instruction selection phase, the resulting target
 * code has to be further processed by a real register allocator.
 */
    class AbstractRegAllocator : public RegAllocator {
    public:
        AbstractRegAllocator()
                : RegAllocator() {
            nextRegIndex_ = 0; // General Purpose registers start from 0
        }

        Reg allocate() override {
            return Reg(Reg::Type::GP, nextRegIndex_++);
        }

    private:
        int nextRegIndex_;
    };

}