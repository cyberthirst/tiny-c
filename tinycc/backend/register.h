//
// Created by Mirek Škrabal on 19.06.2023.
//

#pragma once

#include <string>
#include <climits> // for INT_MAX

class Reg {
public:
    enum class Type {
        SP,
        BP,
        GP,
    };

    Reg(Type type, int index) : type_(type), index_(index) { }

    Type type() const { return type_; }
    int index() const { return index_; }

    std::string toString() const {
        switch(type_) {
            case Type::SP: return "SP";
            case Type::BP: return "BP";
            case Type::GP: return "R" + std::to_string(index_);
            default: return "Invalid";
        }
    }

private:
    Type type_;
    int index_;
};

