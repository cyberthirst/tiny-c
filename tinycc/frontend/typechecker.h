#pragma once

#include "common/types.h"
#include "ast.h"

namespace tiny {

    class TypeError : public std::runtime_error {
    public:
        TypeError(std::string const & what, SourceLocation const & location):
            std::runtime_error{what},
            location_{location} {
        }

        SourceLocation const & location() const {
            return location_;
        }

    private:
        SourceLocation location_;
    };


    /** The typechecker is an AST visitor that walks the AST tree and assigns a type to every node. 

        Failure to assign a type is a type error.  
     */
    class Typechecker : public ASTVisitor {
    public:


        static void checkProgram(std::unique_ptr<AST> const & root) {
            Typechecker t;
            t.typecheck(root);
        }

        // These visitors must be implemented to provide the typechecking functionality. It's best to start adding tests and then implement stuff as you hit the not implemented blocks, or see errors.
        void visit(AST * ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }

        void visit(ASTInteger * ast) override {
            ast->setType(Type::getInt());
        }

        void visit(ASTDouble * ast) override { 
            ast->setType(Type::getDouble());
        }

        void visit(ASTChar * ast) override { 
            ast->setType(Type::getChar());
        }

        void visit(ASTString* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTIdentifier* ast) override { 
            Type * t = getVariable(ast->name);
            if (t == nullptr) 
                throw TypeError("OUCH", ast->location());
            ast->setType(t);
        }
        void visit(ASTType* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }

        void visit(ASTPointerType* ast) override { 
            Type * base = typecheck(ast->base);
            ast->setType(Type::getPointerTo(base));    
        }

        void visit(ASTArrayType* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        
        void visit(ASTNamedType* ast) override { 
            Type * t = Type::getType(ast->name);
            if (t == nullptr) 
                throw TypeError("OUCH", ast->location());
            ast->setType(t);
        }

        void visit(ASTSequence* ast) override { 
            Type * t = Type::getVoid();
            for (auto const & i : ast->body)
                t = typecheck(i);
            ast->setType(t);
        }
        
        void visit(ASTBlock* ast) override { 
            enterBlock();
            for (auto const & i : ast->body) 
                typecheck(i);
            ast->setType(Type::getVoid());    
            leaveBlock();
        }

        // int i; 
        // int i = 67;
        void visit(ASTVarDecl* ast) override { 
            Type * t = typecheck(ast->varType);
            if (ast->value != nullptr) {
                Type * valueType = typecheck(ast->value);
                if (valueType != t) 
                    throw TypeError("OUCH", ast->location());
            }
            addVariable(ast->name->name, t, ast);
            ast->setType(Type::getVoid());
        }
        void visit(ASTFunDecl* ast) override { 
            Type * returnType = typecheck(ast->returnType);
            std::vector<Type*> sig;
            sig.push_back(returnType);
            for (auto & i : ast->args) {
                Type * argType = typecheck(i.first);
                sig.push_back(argType);
            }
            Type * f = Type::getFunction(std::move(sig));
            addVariable(ast->name, f, ast);
            enterFunction(returnType);
            for (auto & i : ast->args)
                addVariable(i.second->name, i.first->type(), i.second.get());
            // TODO
            typecheck(ast->body);
            leaveFunction();
            ast->setType(Type::getVoid());
        }
        void visit(ASTStructDecl* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTFunPtrDecl* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTIf* ast) override { 
            Type * t = typecheck(ast->cond);
            if (t != Type::getInt())
                throw TypeError("", ast->location());
            typecheck(ast->trueCase);
            // HW: can this be nullptr?
            typecheck(ast->falseCase);
            ast->setType(t);
        }
        void visit(ASTSwitch* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTWhile* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTDoWhile* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTFor* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTBreak* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTContinue* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        
        void visit(ASTReturn* ast) override { 
            Type * result = ast->value ? typecheck(ast->value) : Type::getVoid();
            Type * expected = contexts_.back().returnType;
            if (result != expected) 
                throw "OUCH";
            ast->setType(result);
        }

        void visit(ASTBinaryOp* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTAssignment* ast) override { 
            //Type * lhs = typecheck(ast->lvalue);
            //Type * rhs = typecheck(ast->value);
            MARK_AS_UNUSED(ast);
            NOT_IMPLEMENTED;
        }
        void visit(ASTUnaryOp* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTUnaryPostOp* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTAddress* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTDeref* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTIndex* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTMember* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTMemberPtr* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTCall* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTCast* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTPrint* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTScan* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }

    private:

        Typechecker() {
            contexts_.push_back(Context{nullptr});
        }

        struct Context {
            Type * returnType;    
            std::unordered_map<Symbol, Type *> locals;

            Context(Type * returnType):returnType{returnType} {}
        }; 

        void enterFunction(Type * returnType) {
            contexts_.push_back(Context{returnType});
        }

        void enterBlock() {
            contexts_.push_back(Context{contexts_.back().returnType});
        }

        void leaveBlock() {
            contexts_.pop_back();
        }

        void leaveFunction() {
            contexts_.pop_back();
        }

        void addVariable(Symbol name, Type * type, AST * ast) {
            auto & context = contexts_.back();
            auto i = context.locals.find(name);
            if (i != context.locals.end())
                throw TypeError("OUCH", ast->location());
            context.locals.insert(std::make_pair(name, type));
        }

        Type * getVariable(Symbol name) {
            for (size_t i = contexts_.size() - 1, e = contexts_.size(); i < e; --i) {
                auto it = contexts_[i].locals.find(name);
                if (it != contexts_[i].locals.end())
                    return it->second;
            }
            return nullptr;
        }

        template<typename T> 
        typename std::enable_if<std::is_base_of<AST, T>::value, Type *>::type
        typecheck(std::unique_ptr<T> const & child) {
            visitChild(child.get());
            Type * result = child->type();
            if (result == nullptr) 
               throw TypeError("OUCH", child->location());
            return result;
        }



        std::vector<Context> contexts_;


    }; // tiny::Typechecker


} // namespace tiny