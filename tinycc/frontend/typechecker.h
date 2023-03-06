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
        void visit(ASTIdentifier* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTType* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTPointerType* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTArrayType* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTNamedType* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }

        void visit(ASTSequence* ast) override { 
            Type * t = Type::getVoid();
            for (auto const & i : ast->body)
                t = typecheck(i);
            ast->setType(t);
        }
        
        void visit(ASTBlock* ast) override { 
            for (auto const & i : ast->body) 
                typecheck(i);
            ast->setType(Type::getVoid());    
        }

        void visit(ASTVarDecl* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTFunDecl* ast) override { 
            // TODO
            typecheck(ast->body);

            ast->setType(Type::getVoid());
        }
        void visit(ASTStructDecl* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTFunPtrDecl* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTIf* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTSwitch* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTWhile* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTDoWhile* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTFor* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTBreak* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTContinue* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
        void visit(ASTReturn* ast) override { MARK_AS_UNUSED(ast); NOT_IMPLEMENTED; }
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

        struct Context {

        }; 


        template<typename T> 
        typename std::enable_if<std::is_base_of<AST, T>::value, Type *>::type
        typecheck(std::unique_ptr<T> const & child) {
            visitChild(child);
            Type * result = child->type();
            if (result == nullptr) 
               throw TypeError("OUCH", child->location());
            return result;
        }


    }; // tiny::Typechecker


} // namespace tiny