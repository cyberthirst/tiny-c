//
// Created by Mirek Škrabal on 21.06.2023.
//

#pragma once

#include <unordered_map>
#include "optimizer/il.h"

namespace tiny {
/*
 * A stack allocator that allocates variables on the stack.
 * It is used to allocate local variables in functions, keeps
 * track of the offsets of the variables in the stack frame.
 *
 * Currently, a very stupid strategy is used. Each local var
 * is allocated on the stack, we ignore scoping, so if a variable
 * is out of scope, we don't reuse it's space. This is a tradeoff,
 * since if we wanted an effective strategy, we would need liveness
 * and points-to analyses.
 */
    class StackAllocator {
    public:
        //
        StackAllocator() : offset_(8) {}

        int allocate(il::Instruction const *var, size_t size) {
            offsets_.emplace(var, offset_);
            offset_ += size;
            return -offsets_.at(var);
        }

        int getOffset(il::Instruction const *var) const {
            return -offsets_.at(var);
        }

        int getStackSize() const {
            return offset_ - 8;
        }

    private:
        int offset_;
        std::unordered_map<il::Instruction const *, int> offsets_;
    };
}