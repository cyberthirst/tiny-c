//
// Created by Mirek Škrabal on 19.06.2023.
//

#pragma once

#include <string>
#include <climits> // for INT_MAX

namespace tiny::t86 {
    class Reg {
    public:
        enum class Type {
            SP,
            BP,
            GP,
        };

        Reg(Type type, int index) : type_(type), index_(index), physical_(false) {}
        Reg(Type type, int index, bool physical) : type_(type), index_(index), physical_(physical) {}

        Reg() {}

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

        bool operator==(const Reg& other) const {
            // replace these with actual comparison logic based on your Reg definition
            // this is just an example
            return this->type() == other.type() && this->index() == other.index() && this->physical() == other.physical();
        }

        bool physical() const {
            if (type_ == Type::SP || type_ == Type::BP) {
                assert(physical_);
            }

            return physical_;
        }

        void setPhysical(int index) {
            physical_ = true;
            index_ = index;
        }

    private:
        Type type_;
        int index_;
        bool physical_;
    };


}