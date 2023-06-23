//
// Created by Mirek Škrabal on 21.06.2023.
//

#pragma once

#include <unordered_map>
#include "optimizer/il.h"

namespace tiny {
// when we allocate a variable on the stack, we need to skip the BP
// which starts the stack frame - so the first variable can be at [BP-1]
#define SKIP_BP_OFFSET 1
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
        StackAllocator() : offset_(SKIP_BP_OFFSET) {}

        size_t normalize(size_t size) {
            if (size <= 8)
                return 1;
            else {
                //we can't handle variables larger than 8 bytes
                NOT_IMPLEMENTED;
            }
        }

        int allocate(il::Instruction const *var, size_t size) {
            offsets_.emplace(var, offset_);
            offset_ += normalize(size);
            return -offsets_.at(var);
        }

        int getOffset(il::Instruction const *var) const {
            return -offsets_.at(var);
        }

        int getStackSize() const {
            return offset_ - SKIP_BP_OFFSET;
        }

    private:
        int offset_;
        std::unordered_map<il::Instruction const *, int> offsets_;
    };
}