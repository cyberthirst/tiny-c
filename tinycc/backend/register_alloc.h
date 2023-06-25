//
// Created by Mirek Škrabal on 25.06.2023.
//

#pragma once

#include "register.h"
#include "program_structures.h"

namespace tiny::t86 {

    class RegAllocator {
    public:
        RegAllocator()
                : SP(Reg::Type::SP, INT_MAX),
                  BP(Reg::Type::BP, INT_MAX - 1),
                  EAX(Reg::Type::GP, 0) {}

        virtual Reg allocate() = 0;  // Pure virtual function

        const Reg &getSP() const { return SP; }

        const Reg &getBP() const { return BP; }

        const Reg &getEAX() const { return EAX; }

    protected:
        Reg SP;  // Stack Pointer register
        Reg BP;  // Base Pointer register
        Reg EAX; // Return value register
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
            //we reserve the REG0 as EAX for return values
            nextRegIndex_ = 1; // General Purpose registers start from 1
        }

        Reg allocate() override {
            return Reg(Reg::Type::GP, nextRegIndex_++);
        }

    private:
        int nextRegIndex_;
    };

/*
 * $$$$$$$$$$$$$$$ BASIC EXPLANATION $$$$$$$$$$$$$$$
 * plan of attack
 * 1. we need to keep track of liveness of variables
 *   - because we only consider local register allocation, this should be easy and we won't
 *     need any data flow analysis
 *     - local register allocation is concerned only with the current basic block, so we don't need
 *       to manage how the state transforms across different control flow paths
 * 2. we need to detect 2 things:
 *   a) variable definition - that is the assignment of a value to a variable
 *   b) variable use - that is the use of a variable in an expression
 * 3. definition of liveness of a variable:
 *   - a variable is live if it will be read in the future, ie the value is read before it is overwritten
 * 4. basic approach for computing the live variables:
 *   - start from the last instruction in the basic block and have empty set of live variables
 *   - then for each instruction:
 *     - remove target of the instruction
 *     - add all operands of the instruction
 *   - construct one set for each instruction
 * 5. Belady's algorithm:
 *  - when a variable is to be used it is assigned a physical register
 *  - however, when no physical registers are available, the variable with the furthest use is evicted
 *    from the register
 *    - that is the variable that is used furthest in the future will be spilled to memory
 *
 */

    class BeladyRegAllocator : public RegAllocator {
    public:
    private:

    };

    class RndRegAllocator : public RegAllocator {
    public:
        RndRegAllocator(t86::Program &program) : p_(program) {}

        Reg allocate() override {

        }

    private:
        void allocatePhysicalRegs() {
            for (auto& [funName, function] : p_.getFunctions()) {
                auto &basicBlocks = function->getBasicBlocks();
                for (auto &bb: basicBlocks) {
                    for (auto &i: bb->getInstructions()) {
                        /*for (auto &op: i->getOperands()) {
                            if (op->isVariable()) {
                                auto var = dynamic_cast<Variable *>(op.get());
                                if (var->isGlobal()) {
                                    //TODO
                                } else {
                                    //TODO
                                }*/
                            }
                        }
                    }
            }

        t86::Program &p_;
    };
}