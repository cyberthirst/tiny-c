#pragma once

#include <unordered_map>


#include "symbol.h"


namespace tiny {

    class PointerType;
    class FunctionType;

    /** Basic type class. 
     */
    class Type {
    public:

        virtual ~Type() = default;

    }; // tiny::Type



}