#pragma once

#include "common/types.h"
#include "frontend/ast.h"
#include "il.h"

namespace tiny {

    class ASTToILTranslator : public ASTVisitor {
    public:

        static Program translateProgram(std::unique_ptr<AST> const & root) {
            ASTToILTranslator t;
            t.translate(root);
            return std::move(t.p_); 
        }

        void visit(AST * ast) override { 
            MARK_AS_UNUSED(ast); 
            UNREACHABLE;
        }

        /** Translating the program simply means translating all its statements in order. 
         */
        void visit(ASTProgram * ast) override {
            for (auto & s : ast->statements)
                translate(s);
        }
        
        /** Translating the literals is trivial. Since for simplicity reasons we do not allow literals to appear as
         *  arguments of operator instructions, each literal has to be first loaded as an immediate value to a new register.
         */
        void visit(ASTInteger * ast) override { 
            (*this) += LDI(RegType::Int, ast->value, ast);
        }

        void visit(ASTDouble * ast) override { 
            (*this) += LDF(RegType::Float, ast->value, ast);
        }

        void visit(ASTChar * ast) override { 
            (*this) += LDI(RegType::Int, ast->value, ast);
        }

        /** Translating string literals is a bit harder - each string literal is deduplicated and stored as a new
         *  global variable that is also initialized with the contents of the literal.
         */
        void visit(ASTString* ast) override { 
            MARK_AS_UNUSED(ast);
            NOT_IMPLEMENTED;
        }

        /** Identifier is translated as a variable read. Note that this is as the address. 
        */
        void visit(ASTIdentifier* ast) override { 
            // TODO How to deal with structures? 
            lastResult_ = getVariable(ast->name);
            if (lValue_) 
                lValue_ = false;
            else
                (*this) += LD(registerTypeFor(ast->type()), lastResult_, ast);
            ASSERT(lastResult_ != nullptr);
        }

        /** We do not have to worry about types at this point. They have already been analyzed by the typechecker
         *  so this code is unreachable.
         */
        void visit(ASTType* ast) override { MARK_AS_UNUSED(ast); UNREACHABLE; }
        void visit(ASTPointerType* ast) override { MARK_AS_UNUSED(ast); UNREACHABLE; } 
        void visit(ASTArrayType* ast) override { MARK_AS_UNUSED(ast); UNREACHABLE; } 
        void visit(ASTNamedType* ast) override { MARK_AS_UNUSED(ast); UNREACHABLE; }

        /** Sequence simply translates its elements.
         */
        void visit(ASTSequence* ast) override { 
            for (auto & i : ast->body) 
                translate(i);
        }

        /** */
        void visit(ASTBlock* ast) override { 
            ASSERT(f_ != nullptr);
            enterBlock();
            for (auto & i : ast->body) 
                translate(i);
            leaveBlock();
        }


        void visit(ASTVarDecl* ast) override {
            Instruction *lvalue = addVariable(ast->name->name, static_cast<int64_t>(ast->type()->size()));
            if (ast->value) {
                translate(ast->value);
                (*this) += ST(lvalue, lastResult_, ast);
            }
            else {
                //TODO initialize to default value based on the type?
            }

        }
        
        /** Enter a new function.
         */
        void visit(ASTFunDecl* ast) override { 
            Function * f = enterFunction(ast->name);
            for (size_t i = 0, e = ast->args.size(); i != e; ++i) {
                Symbol name = ast->args[i].second->name;
                Instruction *arg = ARG(registerTypeFor(ast->args[i].first->type()),
                                       static_cast<int64_t>(i), ast->args[i].first.get(),
                                       name.name());
                f->addArg(arg);
                // now we need to create a local copy of the value so that it acts as a variable
                Type * t = ast->args[i].first->type();
                if (t->isPointer() || t->isNumeric()) {
                    Instruction * addr = addVariable(name, static_cast<int64_t>(ast->args[i].first->type()->size()));
                    (*this) += ST(addr, arg);
                } else {
                    contexts_.back().locals.insert(std::make_pair(name, arg));
                }
            }
            translate(ast->body);
            if (! bb_->terminated())
                (*this) += RET();
            leaveFunction();
        }

        /** Nothing to do for a struct declaration in the translation phase, the type has been created by the typechecker already.
         */ 
        void visit(ASTStructDecl* ast) override { MARK_AS_UNUSED(ast); lastResult_ = nullptr; }

        /** Nothing to do for a function ptr declaration in the translation phase, the type has already been created by the typechecker.
         */
        void visit(ASTFunPtrDecl* ast) override {MARK_AS_UNUSED(ast); lastResult_ = nullptr; }

        void visit(ASTIf* ast) override {
            translate(ast->cond);

            Instruction *cond = lastResult_;
            BasicBlock *originalBB = bb_;
            BasicBlock *thenBB = f_->addBasicBlock(STR("then"));
            BasicBlock *elseBB = f_->addBasicBlock(STR("else"));
            BasicBlock *mergeBB = f_->addBasicBlock(STR("if-else-merge"));

            //TODO maybe add something like TEST? or something similar like icmp in LLVM?
            Instruction *isZero = lastResult_;
            (*this) += BR(isZero, thenBB, elseBB, ast);

            // Process 'then' block
            //(*this) += JMP(thenBB, ast);
            enterBasicBlock(thenBB);
            translate(ast->trueCase);

            //TODO we enter the basic block here, but if the if-else contain blocks themself
            // then we jump to this BB and inside the block we just enter a new BB
            // will probably be easy to optimize away later
            // Process 'else' block
            if (ast->falseCase) {
                enterBasicBlock(elseBB);
                translate(ast->falseCase);
            }

            // Control flow merges here
            (*this) += JMP(mergeBB, ast);
            enterBasicBlock(mergeBB);
        }

        void visit(ASTSwitch* ast) override { 
            MARK_AS_UNUSED(ast);
            NOT_IMPLEMENTED;
        }

        void visit(ASTWhile* ast) override { 
            MARK_AS_UNUSED(ast);
            NOT_IMPLEMENTED;
        }

        void visit(ASTDoWhile* ast) override { 
            MARK_AS_UNUSED(ast);
            NOT_IMPLEMENTED;

        }

        void visit(ASTFor* ast) override { 
            MARK_AS_UNUSED(ast);
            NOT_IMPLEMENTED;
        }

        void visit(ASTBreak* ast) override { 
            MARK_AS_UNUSED(ast);
            NOT_IMPLEMENTED;
        }

        void visit(ASTContinue* ast) override { 
            MARK_AS_UNUSED(ast);
            NOT_IMPLEMENTED;
        }

        void visit(ASTReturn* ast) override {
            translate(ast->value);
            (*this) += RETR(lastResult_, ast);
        }
        
        void visit(ASTBinaryOp* ast) override { 
            Instruction * lhs = translate(ast->left);
            Instruction * rhs = translate(ast->right);
            // TODO TODO 
            if (ast->op == Symbol::Mul) {
                (*this) += MUL(binaryResult(lhs, rhs), lhs, rhs, ast);
            } else {
                NOT_IMPLEMENTED;
            }
        }

        void visit(ASTAssignment* ast) override { 
            Instruction * lvalue = translateLValue(ast->lvalue);
            Instruction * value = translate(ast->value);
            (*this) += ST(lvalue, value, ast);
        }

        void visit(ASTUnaryOp* ast) override { 
            MARK_AS_UNUSED(ast);
            NOT_IMPLEMENTED;
        }

        void visit(ASTUnaryPostOp* ast) override { 
            MARK_AS_UNUSED(ast);
            NOT_IMPLEMENTED;
        }

        void visit(ASTAddress* ast) override { 
            MARK_AS_UNUSED(ast);
            NOT_IMPLEMENTED;
        }

        void visit(ASTDeref* ast) override { 
            MARK_AS_UNUSED(ast);
            NOT_IMPLEMENTED;
        }

        void visit(ASTIndex* ast) override { 
            MARK_AS_UNUSED(ast);
            NOT_IMPLEMENTED;
        }

        void visit(ASTMember* ast) override { 
            MARK_AS_UNUSED(ast);
            NOT_IMPLEMENTED;
        }

        void visit(ASTMemberPtr* ast) override { 
            MARK_AS_UNUSED(ast);
            NOT_IMPLEMENTED;
        }

        void visit(ASTCall* ast) override { 
            MARK_AS_UNUSED(ast);
            NOT_IMPLEMENTED;
        }

        void visit(ASTCast* ast) override { 
            MARK_AS_UNUSED(ast);
            NOT_IMPLEMENTED;
        }

        void visit(ASTPrint* ast) override { 
            MARK_AS_UNUSED(ast);
            NOT_IMPLEMENTED;
        }

        void visit(ASTScan* ast) override { 
            MARK_AS_UNUSED(ast);
            NOT_IMPLEMENTED;
        }

    private:

        template<typename T> 
        typename std::enable_if<std::is_base_of<AST, T>::value, Instruction *>::type
        translate(std::unique_ptr<T> const & child) {
            visitChild(child.get());
            return lastResult_;
        }   

        template<typename T> 
        typename std::enable_if<std::is_base_of<AST, T>::value, Instruction *>::type
        translateLValue(std::unique_ptr<T> const & child) {
            bool old = lValue_;
            lValue_ = true;
            visitChild(child.get());
            return lastResult_;
            lValue_ = old;
        }   

        /** Adds the given instruction to the program, adding it to the current basic block, which should not be terminated. 
         */
        ASTToILTranslator & operator += (Instruction * ins) {
            ASSERT(bb_ != nullptr);
            bb_->append(ins);
            lastResult_ = ins;
            return *this;
        }

        struct Context {
            std::unordered_map<Symbol, Instruction *> locals;
            BasicBlock * localsBlock = nullptr;

            Context(BasicBlock * locals):
                localsBlock{locals} {
            }

        }; // ASTToILTranslator::Context

        RegType registerTypeFor(Type * t) {
            return t == Type::getDouble() ? RegType::Float : RegType::Int;            
        }


        RegType binaryResult(Instruction * lhs, Instruction * rhs) {
            ASSERT(lhs->type == rhs->type && "We need identical types on lhs and rhs");
            return lhs->type;
        }

        Function * enterFunction(Symbol name) {
            ASSERT(f_ == nullptr);
            f_ = p_.addFunction(name);
            Instruction * fReg = FUN(name, name.name());
            p_.globals()->append(fReg);
            bb_ = f_->addBasicBlock("prolog");
            contexts_.push_back(Context{bb_});
            contexts_.front().locals.insert(std::make_pair(name, fReg));
            return f_;
        }

        void leaveFunction() {
            f_ = nullptr;
        }

        /** Enters new block. 


         */
        void enterBlock(std::string const & name = "") {
            BasicBlock * bb = f_->addBasicBlock(name);
            if (! bb_->terminated())
                bb_->append(JMP(bb));
            bb_ = bb;
            contexts_.push_back(Context{bb_});
        }

        void leaveBlock() {
            contexts_.pop_back();
        }

        BasicBlock *enterBasicBlock(BasicBlock * bb) {
            assert(bb && "null basic block");
            assert(!bb->terminated() && "basic block already terminated");
            bb_ = bb;
            return bb_;
        }

        /** Creates new local variable with given name and size. The variable's ALLOCA instruction is appended to the
         *  current block's local definitions basic block and the register containing the address is returned.
         */
        Instruction * addVariable(Symbol name, size_t size) {
            Instruction * res = contexts_.back().localsBlock->append( ALLOCA( RegType::Int,
                                                                              static_cast<int64_t>(size), name.name()));
            contexts_.back().locals.insert(std::make_pair(name, res));
            return res;
        }

        /** Returns the register that holds the address of variable with given name. The address can then be used to
         *  load/store its contents.
         */
        Instruction * getVariable(Symbol name) {
            for (size_t i = contexts_.size() - 1, e = contexts_.size(); i < e; --i) {
                auto it = contexts_[i].locals.find(name);
                if (it != contexts_[i].locals.end())
                    return it->second;
            }
            return nullptr;
        ;}

        Program p_;
        std::vector<Context> contexts_;
        Instruction * lastResult_ = nullptr;
        BasicBlock * bb_ = nullptr;
        Function * f_ = nullptr; 
        bool lValue_ = false;


    }; // tiny::ASTToILTranslator

} // namespace tiny