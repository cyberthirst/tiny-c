//
// Created by Mirek Škrabal on 21.06.2023.
//

#pragma once

#include <memory>

#include "operand.h"
#include "common/colors.h"
#include "t86_instruction.h"


namespace tiny::t86 {
    class BasicBlock {
    public:

        std::string const name;

        BasicBlock():
                name{makeUniqueName()} {
        }

        /*BasicBlock(std::string const & name):
                name{makeUniqueName(name)} {
        }*/
        BasicBlock(std::string const & name): name{name} {}
        /*bool terminated() const {
            if (insns_.empty())
                return false;
            return dynamic_cast<Instruction::Terminator*>(insns_.back().get()) != nullptr;
        }*/

        /** Appends the instruction to the given basic block.
         */
        Instruction * append(Instruction * ins) {
            insns_.push_back(std::unique_ptr<Instruction>{ins});
            return ins;
        }

        size_t size() const { return insns_.size(); }

        Instruction * operator[](size_t i) const { return insns_[i].get(); }

        const std::vector<std::unique_ptr<Instruction>>& getInstructions() const {
            return insns_;
        }

    private:

        friend class Function;
        friend class Program;

        std::string toString() const {
            std::stringstream s;
            s << "# bb: " << name << ":";
            for (auto & i : insns_) {
                s << "\n";
                //TODO print the instruction
                //i->print(p);
                s << i->toString();
            }
            return s.str();
        }


        static std::string makeUniqueName() {
            return STR("bb" << nextUniqueId());
        }

        static std::string makeUniqueName(std::string const & from) {
            return STR(from << nextUniqueId());
        }

        static size_t nextUniqueId() {
            static size_t i = 0;
            return i++;
        }

        std::vector<std::unique_ptr<Instruction>> insns_;
    };


    class Function {
    public:
        BasicBlock * addBasicBlock() {
            std::unique_ptr<BasicBlock> bb{new BasicBlock{}};
            bbs_.push_back(std::move(bb));
            return bbs_.back().get();
        }

        BasicBlock * addBasicBlock(std::string const & name) {
            std::unique_ptr<BasicBlock> bb{new BasicBlock{name}};
            bbs_.push_back(std::move(bb));
            return bbs_.back().get();
        }


        Instruction * addArg(Instruction * arg) {
            std::unique_ptr<Instruction> a{arg};
            args_.push_back(std::move(a));
            return arg;
        }

        size_t numArgs() const { return args_.size(); }

        //Instruction const * getArg(size_t i) const { return args_[i].get(); }

        const std::vector<std::unique_ptr<BasicBlock>>& getBasicBlocks() const { return bbs_; }

        std::vector<std::unique_ptr<BasicBlock>>& getBasicBlocks() { return bbs_; }

        std::string toString() const {
            std::stringstream s;
            //TODO print args
            for (auto & bb : bbs_) {
                s << bb->toString();
            }
            return s.str();
        }

        BasicBlock * start() const { return bbs_[0].get(); }
        //RegType retType_;
    private:
        std::vector<std::unique_ptr<Instruction>> args_;
        std::vector<std::unique_ptr<BasicBlock>> bbs_;
    };

    class Program {
    public:
        Program() = default;
        ~Program() = default;

        Function * addFunction(Symbol name){
            if (functions_.find(name) != functions_.end()) {
                throw std::runtime_error(STR("function " << name << " already exists"));
            }
            Function * f = new Function{};
            functions_.insert(std::make_pair(name, f));
            return f;
        }

        std::unordered_map<Symbol, Function *> &getFunctions() { return functions_; }

        std::string toString() const {
            std::stringstream ss;
            ss << emitStart();
            for (auto &f : functions_) {
                ss << "# " << f.first << "\n";
                ss << f.second->toString();
            }
            ss << '\n';
            return ss.str();
        }

        std::string emitStart() const {
            std::stringstream ss;
            ss << ".text" << "\n";
            ss << "#.global main" << "\n";
            //calls main, outputs the result and halts
            //TODO make this code, not just a string
            ss << "CALL 3\nPUTNUM R0\nHALT\n";
            return ss.str();
        }


    private:
        std::unordered_map<Symbol, Function *> functions_;
    };
}