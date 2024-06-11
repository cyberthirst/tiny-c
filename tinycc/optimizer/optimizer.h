#pragma once
#include <cstdint>
#include <unordered_map>
#include "il.h"
#include "peephole.h"

namespace tiny {

    class BackendOptimizer {
    public:
        static void optimize(t86::Program& program) {
            BackendOptimizer opt;
            bool changed = false;
            do {
                changed = false;
                changed |= PeepholeOptimizer::optimize(program);
            } while (changed);
        }
    private:
        BackendOptimizer() = default;
    };

    class MiddleEndOptimizer {
    public:
        static void optimize(il::Program &program) {
            MiddleEndOptimizer opt;
            opt.removeRedundantJMPBBs(program);
        }

    private:
        MiddleEndOptimizer() = default;
        //some BBs only contain a JMP instruction, this function removes them
        static void removeRedundantJMPBBs(il::Program& program) {
            std::unordered_map<il::BasicBlock *, il::BasicBlock *> redundantBlocks;
            // Step 1: Identify redundant blocks
            for (const auto &[name, function]: program.getFunctions()) {
                for (const auto &bbPtr: function->getBasicBlocks()) {
                    if (bbPtr->size() == 1) {  // Check if only one instruction
                        auto *terminatorB = dynamic_cast<il::Instruction::TerminatorB *>(bbPtr->operator[](0));
                        // Check if the single instruction is a JMP
                        if (terminatorB && terminatorB->opcode == il::Opcode::JMP) {
                            redundantBlocks[bbPtr.get()] = terminatorB->target;
                        }
                    }
                }
            }

            bool changed = true;
            while (changed) {
                changed = false;
                // Step 2: Redirect JMPs and BRs
                for (const auto &[name, function]: program.getFunctions()) {
                    for (const auto &bbPtr: function->getBasicBlocks()) {
                        for (size_t i = 0; i < bbPtr->size(); ++i) {
                            auto *terminatorB = dynamic_cast<il::Instruction::TerminatorB *>(bbPtr->operator[](i));
                            if (terminatorB && terminatorB->opcode == il::Opcode::JMP) {
                                auto found = redundantBlocks.find(terminatorB->target);
                                // If the current JMP target is a redundant block, redirect the JMP
                                if (found != redundantBlocks.end()) {
                                    terminatorB->target = found->second;
                                    changed = true;
                                }
                                continue;
                            }
                            auto *terminatorRegBB = dynamic_cast<il::Instruction::TerminatorRegBB *>(bbPtr->operator[](
                                    i));
                            if (terminatorRegBB && terminatorRegBB->opcode == il::Opcode::BR) {
                                auto found1 = redundantBlocks.find(terminatorRegBB->target1);
                                if (found1 != redundantBlocks.end()) {
                                    terminatorRegBB->target1 = found1->second;
                                    changed = true;
                                }
                                auto found2 = redundantBlocks.find(terminatorRegBB->target2);
                                if (found2 != redundantBlocks.end()) {
                                    terminatorRegBB->target2 = found2->second;
                                    changed = true;
                                }
                            }
                        }
                    }
                }
            }

            // Step 3: remove all the redundant blocks from each function
            for (const auto& [name, function] : program.getFunctions()) {
                function->getBasicBlocks().erase(std::remove_if(function->getBasicBlocks().begin(),
                                                                function->getBasicBlocks().end(),
                                                                [&](const std::unique_ptr<il::BasicBlock>& bb) {
                                                                    return redundantBlocks.count(bb.get()) > 0;
                                                                }),
                                                 function->getBasicBlocks().end());
            }
        }

    };


    class Optimizer {
    public:
        static void optimize(il::Program &program) {
            MiddleEndOptimizer::optimize(program);
        }

        static void optimize(t86::Program &program) {
            BackendOptimizer::optimize(program);
        }
    }; // tiny::Optimizer





    template<typename T>
    class State {
    public:

        T & get(il::Instruction * reg) {
            return state_[reg]; // create bottom if does not exist yet
        }

        void set(il::Instruction * reg, T val) {
            state_[reg] = val;
        }

        bool mergeWith(State<T> const & other) {
            // merge all held values, including those onl in the orher state
            // TODO
            NOT_IMPLEMENTED;
        }

    private:
        std::unordered_map<il::Instruction *, T> state_;

    }; // tiny::State



    template<typename T>
    class ForwardAnalysis {
    public:

        void analyze(il::BasicBlock * start, State<T> initialState) {
            q_.push_back(start, initialState);
            while (!q_.empty()) {
                // get next basic block to analyze
                il::BasicBlock * b = q_.back();
                q_.pop_back();
                State<T> state = inputStates_[b];
                // process all instructions
                for (size_t i = 0, e = b->size(); i != e; ++i)
                    processInstruction(b[i], state);
                // add next states
                // TODO (depends on your API for terminators)
                // pseudocode: for all next blocks: analyzeBB(next, state);
            }
        }


    protected:

        void analyzeBB(il::BasicBlock * b, State<T> const & inputState) {
            if (inputStates_[b].mergeWith(inputState))
                q_.push_back(b);
        }

        virtual void processInstruction(il::Instruction * ins, State<T> & state) = 0;

        std::vector<il::BasicBlock *> q_;
        std::unordered_map<il::BasicBlock *, State<T>> inputStates_;
    }; // tiny::ForwardAnalysis


    struct SimpleCPValue {
        enum class Kind {
            Bottom,
            Constant,
            NonZero,
            Top,
        };
        Kind kind;
        int64_t value = 0;

        SimpleCPValue(): kind{Kind::Bottom} {}

        /** Merges the value with other. Returns true if there was change, false otherwise.
         */
        bool mergeWith(SimpleCPValue const & other) {
            // if we are bottom, take the other value
            if (kind == Kind::Bottom) {
                kind = other.kind;
                value = other.value;
                return kind != Kind::Bottom;
            };
            // if we are top, we can't change
            if (kind == Kind::Top)
                return false;
            // if we are the same, nothing changes
            if (*this == other)
                return false;
            // if the other one is top, we change to top
            if (other.kind == Kind::Top) {
                kind = Kind::Top;
                return true;
            }
            // if the other one is bottom, then we don't change
            if (other.kind == Kind::Bottom)
                return false;
            // TODO the rest
            NOT_IMPLEMENTED;
        }

        bool operator == (SimpleCPValue const & other) const {
            return kind == other.kind && (kind != Kind::Constant || (value == other.value));
        }
    }; // tiny::SimpleCPValue

    class CPAnalysis : public ForwardAnalysis<SimpleCPValue> {
    public:


    protected:

        void processInstruction(il::Instruction * ins, State<SimpleCPValue> & state) override {
            switch (ins->opcode) {
                case il::Opcode::ADD:
                    // TODO
                    break;
            }
            NOT_IMPLEMENTED;
        }

    }; // CPAnalysis

} // namespace tiny

