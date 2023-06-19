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

