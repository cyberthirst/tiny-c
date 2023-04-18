#pragma once

#include "common/types.h"
#include "common/source_error.h"
#include "ast.h"

namespace tiny {

    class TypeError : public SourceError {
    public:
        TypeError(std::string const & what, SourceLocation const & location):
            SourceError{"TypeError", what, location} {
        }
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

        /** It's ok to leave this unimplemented, the generic AST visitor only exists for fallbacks cases which we do not use in the typechecker. 
        */
        void visit(AST * ast) override { MARK_AS_UNUSED(ast); UNREACHABLE; }
        
        void visit(ASTProgram * ast) override { 
            for (auto & s : ast->statements)
                typecheck(s);
            ast->setType(Type::getVoid());
        }

        void visit(ASTInteger * ast) override { 
            ast->setType(Type::getInt());
        }

        void visit(ASTDouble * ast) override { 
            ast->setType(Type::getDouble());
        }
        
        void visit(ASTChar * ast) override { 
            ast->setType(Type::getChar());
        }
        
        void visit(ASTString * ast) override { 
            ast->setType(Type::getPointerTo(Type::getChar()));
        }

        /** Verify the variable exists, raise an error if not, otherwise set type the type of the variable.
         */
        void visit(ASTIdentifier * ast) override { 
            Type * t = getVariable(ast->name);
            if (t == nullptr)
                throw TypeError{STR("Unknown variable " << ast->name.name()), ast->location()};
            ast->setType(t);
        }            
        
        /** Actually unreachable, handled by the special cases below.
         */
        void visit(ASTType * ast) override { MARK_AS_UNUSED(ast); UNREACHABLE; }

        /** Create the pointer type. 
         */
        void visit(ASTPointerType * ast) override { 
            Type * base = typecheck(ast->base);
            ast->setType(Type::getPointerTo(base));
        } 

        /** A bit hacky, we treat arrays equally to pointers, i.e. in tinyc the sizeof array will be sizeof ptr still.
         *  A proper way would be to have an array type that would also keep the size with it and be convertible to a pointer.  
         */
        void visit(ASTArrayType * ast) override { 
            Type *sz = typecheck(ast->size);
            Type *b = typecheck(ast->base);
            if (!sz->isIntegral())
                throw TypeError{STR("Array base type must be integral, not " << *b), ast->location()};
            ast->setType(Type::getPointerTo(b));
        }
        
        /** For a named type, we need it to be present in the known types, otherwise it is a failure. 
         */
        void visit(ASTNamedType * ast) override { 
            Type * t = Type::getType(ast->name);
            if (t == nullptr) 
                throw TypeError{STR("Unknown type " << ast->name), ast->location()};
            ast->setType(t);
        }

        /** Sequence typechecks its elements and unlike block makes sure its own type is that of the last element. 
         */
        void visit(ASTSequence * ast) override { 
            Type * t = nullptr;
            for (auto & i : ast->body)
                t = typecheck(i);
            ast->setType(t);    
        }
        
        /** Typechecks a block of statements. Typechecks each part of the block and returns the void type.
         */
        void visit(ASTBlock * ast) override { 
            enterBlock();
            for (auto const & child : ast->body)
                typecheck(child);
            ast->setType(Type::getVoid());
            leaveBlock();
        }
        
        /** Variable declaration gets the variable type, if there is a value specified for it ensures the value type
         *  corresponds to the variable type and then adds the variable to local context. If the variable already
         *  exists in current context (we allow shadowing), an error is raised. 
         */
        void visit(ASTVarDecl * ast) override { 
            Type * t = typecheck(ast->varType);
            if (!t->isFullyDefined())
                throw TypeError(STR("Type " << *t << " is not fully defined yet"), ast->location());
            if (ast->value != nullptr) {
                Type * valueType = typecheck(ast->value);
                if (valueType != t && valueType->isConvertibleTo(t)
                    throw TypeError{STR("Value of type " << *valueType << " cannot be assigned to variable of type " << *t), ast->location()};
            }
            addVariable(ast->name->name, t, ast);
            ast->setType(t);
        }        

        /** Typechecking a function simply checks that the body typechecks well.
         */
        void visit(ASTFunDecl * ast) override { 
            // first we need to create the function type signature in the form retType..args. All types used there must be fully
            // defined for the declaration to succeed
            Type * returnType = typecheck(ast->returnType);
            if (! returnType->isFullyDefined())
                throw TypeError{"Function return type must be fully defined", ast->returnType->location()};
            std::vector<Type*> signature;
            signature.push_back(returnType);
            for (auto & i : ast->args) {
                Type * argType = typecheck(i.first);
                if (! argType->isFullyDefined())
                    throw TypeError{"Function argument type must be fully defined", i.first->location()};
                signature.push_back(argType);
            }
            // create the type of the function and add a global variable of the type with the name of the function so
            // that we can get a pointer to it easily
            Type * ftype = Type::getFunction(signature);
            addVariable(ast->name, ftype, ast);
            // now that the function type has been created, we can enter the function, add local variables for its arguments
            // and then typecheck its body
            enterFunction(returnType);
            for (auto & i : ast->args) 
                addVariable(i.second->name, i.first->type(), i.second.get());
            // verify that the function actually returns the type it should have. For this we need the block to return the result of its  
            typecheck(ast->body);
            if (!returned_ && returnType != Type::getVoid()) 
                throw TypeError(STR("Not all paths of the function return " << *returnType), ast->location());
            ast->setType(Type::getVoid());
            leaveFunction();
        }

      
        void visit(ASTStructDecl * ast) override { 
            enterBlock();
            bool isFullyDefined = ast->isDefinition;
            StructType *st = Type::getOrDeclareStruct(ast->name);
            if (st->isFullyDefined() && ast->isDefinition)
                throw TypeError{STR("Struct " << ast->name << " is already defined"), ast->location()};
            for (auto & i : ast->fields) {
                Type *fieldT = typecheck(i.second);
                addVariable(i.first->name,  fieldT, i.second.get());
                isFullyDefined &= typecheck(i.first)->isFullyDefined();
            }
            leaveBlock();

            if (isFullyDefined)
                st->markAsFullyDefined();
            ast->setType(st);
        }

        void visit(ASTFunPtrDecl * ast) override { 
            // first we need to create the function type signature in the form retType..args. All types used there must be fully
            // defined for the declaration to succeed
            Type * returnType = typecheck(ast->returnType);
            if (! returnType->isFullyDefined())
                throw TypeError{"Function return type must be fully defined", ast->returnType->location()};
            std::vector<Type*> signature;
            signature.push_back(returnType);
            for (auto & i : ast->args) {
                Type * argType = typecheck(i);
                if (! argType->isFullyDefined())
                    throw TypeError{"Function argument type must be fully defined", i->location()};
                signature.push_back(argType);
            }
            // create the type of the function and add a global variable of the type with the name of the function so
            // that we can get a pointer to it easily
            Type *t = Type::getPointerTo(Type::getFunction(signature));
            Type::createAlias(ast->name->name, t);
            ast->setType(t);
        }

        void visit(ASTIf * ast) override {
            bool old = returned_;
            if (! typecheck(ast->cond)->convertsToBool())
                throw TypeError{STR("Condition must convert to bool, but " << *ast->cond->type() << " found"), ast->cond->location()};
            returned_ = false;
            typecheck(ast->trueCase);
            bool allReturn = returned_;
            if (ast->falseCase != nullptr) {
                returned_ = false;
                typecheck(ast->falseCase);
                allReturn &= returned_;
            } else {
                allReturn = false;
            }
            ast->setType(Type::getVoid());            
            returned_ = old || allReturn;
        }

        void visit(ASTSwitch * ast) override { 
            MARK_AS_UNUSED(ast);
            NOT_IMPLEMENTED;
        }

        void visit(ASTWhile * ast) override { 
            bool old = returned_;
            if(!typecheck(ast->cond)->convertsToBool())
                throw TypeError{STR("Condition must convert to bool, but " << *ast->cond->type() << " found"), ast->cond->location()};
            typecheck(ast->body);
            ast->setType(Type::getVoid());
            //TODO (applies also to other conds)
            //what if the cond is always true and the body contains a return?
            //then the while loop is guaranteed to return
            //we assume that the cond might sometimes be false and thus the while loop might not return
            returned_ = old;
        }

        void visit(ASTDoWhile * ast) override {
            if(!typecheck(ast->cond)->convertsToBool())
                throw TypeError{STR("Condition must convert to bool, but " << *ast->cond->type() << " found"), ast->cond->location()};
            typecheck(ast->body);
            ast->setType(Type::getVoid());
        }

        void visit(ASTFor * ast) override {
            bool old = returned_;
            typecheck(ast->init);
            if(!typecheck(ast->cond)->convertsToBool())
                throw TypeError{STR("Condition must convert to bool, but " << *ast->cond->type() << " found"), ast->cond->location()};
            typecheck(ast->increment);
            typecheck(ast->body);
            ast->setType(Type::getVoid());
            returned_ = old;
        }

        /** No typechecking here */
        void visit(ASTBreak * ast) override { 
            ast->setType(Type::getVoid());
        }

        /** No typechecking here */
        void visit(ASTContinue * ast) override { 
            ast->setType(Type::getVoid());
        }

        /** Typecheck the expression and then verify that the type corresponds to the function type. 
         */
        void visit(ASTReturn * ast) override { 
            Type * result = ast->value ? typecheck(ast->value) : Type::getVoid();
            Type * expectedReturnType = contexts_.back().returnType;
            if (result != expectedReturnType)
                throw TypeError{STR("Return type " << *result << " found, but " << *expectedReturnType << " found"), ast->location()};
            ast->setType(result);
            returned_ = true; // mark that we have successfully returned
        }

        /** Binary operators are not hard, just a bit of processing is required. We need to support all binary operators
         *  TinyC recognizes, i.e.: *, /, %, +, -, <<, >>, <, <=, >, >=, ==, !=, &, |, && and ||. 
         */
        void visit(ASTBinaryOp * ast) override {
            Type *l =  typecheck(ast->left);
            Type *r =  typecheck(ast->right);
            Type *res = nullptr;
            //logical and && and or ||
            if (ast->op == Symbol::And || ast->op == Symbol::Or) {
                // for && and || we need to check that the left hand side converts to bool and then typecheck the right hand side
                // and then check that the right hand side converts to bool
                if (!l->convertsToBool())
                    throw TypeError{STR("Left hand side of " << ast->op << " must convert to bool, but " << *ast->left->type() << " found"), ast->left->location()};
                if (!r->convertsToBool())
                    throw TypeError{STR("Right hand side of " << ast->op << " must convert to bool, but " << *ast->right->type() << " found"), ast->right->location()};
                ast->setType(Type::getInt());
            }
            else if (ast->op == Symbol::BitAnd || ast->op == Symbol::BitOr) {
                if (l != r) {
                    throw TypeError{STR("Both sides of " << ast->op << " must be of the same type, but " << *ast->left->type() << " and " << *ast->right->type() << " found"), ast->location()};
                }
                if (!l->isIntegral()) {
                    throw TypeError{STR("Both sides of " << ast->op << " must be integral, but " << *ast->left->type() << " and " << *ast->right->type() << " found"), ast->location()};
                }
                ast->setType(l);
            }
            else if (ast->op == Symbol::Mod) {
                if (!l->isIntegral() || !r->isIntegral()) {
                    throw TypeError{STR("Modulo operator can only be applied to integral types, but " << *l << " and " << *r << " found"), ast->location()};
                }
                ast->setType(l);
            }
            else if (ast->op == Symbol::Add || ast->op == Symbol::Sub) {
                if (l->isPointer()) {
                    if (r != Type::getInt()) {
                        throw TypeError{STR("For pointer arithmetic the right hand side must be an integer, but " << *ast->right->type() << " found"), ast->right->location()};
                    }
                    ast->setType(l);
                } 
                else {
                    res = getArithmeticResult(l,r);
                    if (!res) {
                        throw TypeError{STR("Cannot apply " << ast->op << " to " << *l << " and " << *r), ast->location()};
                    }
                    ast->setType(res);
                }
            }
            else if (ast->op == Symbol::Mul || ast->op == Symbol::Div) {
                res = getArithmeticResult(l, r);
                if (!res) {
                    throw TypeError{STR("Cannot apply " << ast->op << " to " << *l << " and " << *r), ast->location()};
                }
                ast->setType(res);
            }
            else if (ast->op == Symbol::Lte || ast->op == Symbol::Gte || ast->op == Symbol::Lt || ast->op == Symbol::Gt) {
                if ((!l->isPointer() && !l->isNumeric()) || (!r->isPointer() && (!r->isNumeric()))) 
                    throw TypeError(STR("Only pointers and numeric types can be compared but " << *l << " and " << *r << " found"), ast->location());
                ast->setType(Type::getInt());
            }
            else if (ast->op == Symbol::Eq || ast->op == Symbol::NEq) {
                if (!l->convertsImplicitlyTo(r) && !(r->convertsImplicitlyTo(l)))
                    throw TypeError(STR("Types " << *l << " and " << *r << " cannot be compared"), ast->location());
                ast->setType(Type::getInt());
            }
            else if (ast->op == Symbol::ShiftRight || ast->op == Symbol::ShiftLeft) {
                if (l != Type::getInt()) {
                    throw TypeError{STR("Left hand side of " << ast->op << " must be an integer, but " << *ast->left->type() << " found"), ast->left->location()};
                }
                if(!r->isIntegral()) {
                    throw TypeError{STR("Right hand side of " << ast->op  <<  " must be integral, but " << *ast->right->type() << " found"), ast->right->location()};
                }
                ast->setType(Type::getInt());
            }
            else {
                //should be handled by the parser - we should never get an unknown operator here
                throw TypeError{STR("Unknown operator " << ast->op), ast->location()};
            }
        }

        /** We must make sure the left hand side of the assignment has an address.
         *  Then ensure that the types match, including any implicit conversions and is ok, set own type to that of the rhs.
        */
        void visit(ASTAssignment * ast) override { 
            Type *lhs = typecheck(ast->lvalue);
            Type *rhs = typecheck(ast->value);
            if (!ast->lvalue->hasAddress())
                throw TypeError{STR("Left hand side (" << *lhs << ") of assignment must have an address"), ast->location()};
            if (!rhs->convertsImplicitlyTo(lhs))
                throw TypeError{STR("Left hand side of assignment " << *lhs << " must have the same type as the right hand side " <<  *rhs), ast->location()};
            ast->setType(rhs);
        }

        /** We have to handle correctly the types for all unary operators in TinyC, i.e. +, -, ~, !, ++ and --. 
         */       
        void visit(ASTUnaryOp * ast) override { 
            Type *t = typecheck(ast->arg);
            if (ast->op == Symbol::Inc || ast->op == Symbol::Dec) {
                if (!ast->arg->hasAddress())
                    throw TypeError{"Pre-increment/decrement requires an addressable value", ast->location()};
                if (!t->isNumeric() && !t->isPointer())
                    throw TypeError{"Pre-increment/decrement requires a numeric or pointer type", ast->location()};
            } else if (ast->op == Symbol::Add || ast->op == Symbol::Sub) {
                if (!t->isNumeric())
                    throw TypeError{"Unary plus/minus requires a numeric type", ast->location()};
            } else if (ast->op == Symbol::Neg) {
                if (!t->isIntegral())
                    throw TypeError{"Unary bitwise not requires an integral type", ast->location()};
            } else if (ast->op == Symbol::Not) {
                if (!t->convertsToBool())
                    throw TypeError{"Unary logical not requires a type that converts to bool", ast->location()};
                ast->setType(Type::getInt());
                return;
            } else {
                throw TypeError{"Unknown unary operator", ast->location()};
            }
            ast->setType(t);
        }

        /** TinyC only has two post-operands, the post-increment and post-decrement. As they both modify the value, they require
         *  it to has an address. Only numeric types and pointers are supported. 
         */
        void visit(ASTUnaryPostOp * ast) override { 
            Type *t = typecheck(ast->arg);
            if (!ast->arg->hasAddress())
                throw TypeError{"Post-increment/decrement requires an addressable value", ast->location()};
            if (!t->isNumeric() && !t->isPointer())
                throw TypeError{"Post-increment/decrement requires a numeric or pointer type", ast->location()};
            ast->setType(t);
        } 
        
        /** The interesting feature of the address operator is that not every value in tinyC has an address (only local variables do)
        */
        void visit(ASTAddress * ast) override { 
            Type *t = typecheck(ast->target);
            if (!ast->target->hasAddress())
                throw TypeError{"Cannot take address of a value that does not have an address", ast->location()};
            ast->setType(Type::getPointerTo(t));
        }

        /** Only pointers can be dereferenced, in which case the result is their base type.
         */
        void visit(ASTDeref * ast) override { 
            Type *t = typecheck(ast->target);
            if (!t->isPointer())
                throw TypeError{"Cannot dereference a non-pointer type", ast->location()};
            auto *ptr = static_cast<PointerType*>(t);
            ast->setType(ptr->base());
        }

        /** Only pointers can be indexed.
         */
        void visit(ASTIndex * ast) override { 
            Type *i = typecheck(ast->index); 
            Type *b = typecheck(ast->base);
            auto *p = dynamic_cast<PointerType*>(b);
            if (!p)
                throw TypeError{"Cannot index a non-pointer type", ast->location()};
            if (!i->isIntegral())
                throw TypeError{"Cannot index with a non-integral type", ast->location()};
            ast->setType(p->base());
        }

        void visit(ASTMember * ast) override { 
            auto * baseType = dynamic_cast<StructType*>(typecheck(ast->base));
            if (baseType == nullptr)
                throw TypeError(STR("Cannot take field from a non-struct type " << *(ast->type())), ast->location());
            if (! baseType->isFullyDefined())
                throw TypeError(STR("Cannot take field from a not fully defined type " << *baseType), ast->location());
            Type * t = (*baseType)[ast->member];
            if (t == nullptr)
                throw TypeError(STR("Struct of type " << *baseType << " does not have field " << ast->member.name()), ast->location());
            ast->setType(t);
        }

        /** Similar to AST member, but we must make sure that where we take the field from is a pointer and dereference it first */
        void visit(ASTMemberPtr * ast) override { 
           auto * p = dynamic_cast<PointerType*>(typecheck(ast->base));
            if (p == nullptr)
                throw TypeError(STR("Only a pointer can appear left of ->, but " << *(ast->type()) << " found"), ast->location());
            auto * baseType = dynamic_cast<StructType*>(p->base());
            if (baseType == nullptr)
                throw TypeError(STR("Cannot take field from a non-struct type " << *(ast->type())), ast->location());
            if (! baseType->isFullyDefined())
                throw TypeError(STR("Cannot take field from a not fully defined type " << *baseType), ast->location());
            Type * t = (*baseType)[ast->member];
            if (t == nullptr)
                throw TypeError(STR("Struct of type " << *baseType << " does not have field " << ast->member.name()), ast->location());
            ast->setType(t);
        }
        
        void visit(ASTCall * ast) override { 
            Type * t = typecheck(ast->function);
            auto * ft = dynamic_cast<FunctionType*>(t);
            if (ft == nullptr) 
                throw TypeError{STR("Expected function, but value of " << *t << " found"), ast->location()};
            if (ast->args.size() != ft->numArgs())
                throw TypeError{STR("Function of type " << *ft << " requires " << ft->numArgs() << " arguments, but " << ast->args.size() << " given"), ast->location()};
            for (size_t i = 0; i < ast->args.size(); ++i) {
                Type * argType = typecheck(ast->args[i]);
                if (argType != ft->arg(i))
                    throw TypeError{STR("Type " << *(ft->arg(i)) << " expected for argument " << (i + 1) << ", but " << *argType << " found"), ast->args[i]->location()};
            }
            ast->setType(ft->returnType());
        }

        /** In C-like languages, from typechecking perspective, casting is really trivial. Anything can typecheck to anything, as long as (a) the value typechecks and
         * (b) the target type is fully defined.   
         */
        void visit(ASTCast * ast) override { 
            typecheck(ast->value);
            Type * target = typecheck(ast->type);
            if (! target->isFullyDefined())
                throw TypeError(STR("Cannot typecheck to incomplete type " << *target), ast->location());
            ast->setType(target);
        }

        /** tinyC only supports writing characters for now. So check we give it character and then type write as void so that it can't be used further.
         */
        void visit(ASTPrint * ast) override { 
            if (typecheck(ast->value) != Type::getChar())
                throw TypeError{STR("Write expects char, but" << *ast->value->type() << " found"), ast->location()};
            ast->setType(Type::getVoid());
        }

        /** Reading returns always a character.
         */
        void visit(ASTScan * ast) override { 
            ast->setType(Type::getChar());
        }

    protected:

        struct Context {
            Type * returnType;
            std::unordered_map<Symbol, Type *> locals;

            Context(Type * returnType = nullptr): returnType{returnType} {}
        }; 

        Typechecker() {
            contexts_.push_back(Context{nullptr}); // the global scope
            Type::resetTypeInformation();
            // add stdlib functions
            Type * int_f = Type::getFunction(std::vector{Type::getInt()});
            Type * void_f_char = Type::getFunction(std::vector{Type::getVoid(), Type::getChar()});
            // ok to pass nullptr here as ast, those are builtins and ast is only used for error reporting, there should be no errors here:)
            addVariable(Symbol{"scan"}, int_f, /* ast */nullptr);
            addVariable(Symbol{"print"}, void_f_char, /* ast */nullptr);
        }

        /** Enters a new function context. 
         * 
        */
        void enterFunction(Type * returnType) {
            contexts_.push_back(Context{returnType});
            returned_ = false;
        }

        void leaveFunction() {
            contexts_.pop_back();
        }

        void enterBlock() {
            contexts_.push_back(Context{contexts_.back().returnType});
        }

        void leaveBlock() {
            contexts_.pop_back();
        }

        Type * getArithmeticResult(Type * a, Type * b) {
            if (!a->isNumeric() || !b->isNumeric())
                return nullptr; // not numbers
            Type * d = Type::getDouble();
            Type * i = Type::getInt();
            if (a == d || b == d)
                return d;
            if (a == i || b == i)
                return i;
            // otherwise it has to be char
            return Type::getChar();
        }

        void addVariable(Symbol name, Type * t, AST * ast) {
            auto & ctx = contexts_.back();
            auto i = ctx.locals.find(name);
            if (i != ctx.locals.end())
                throw ParserError{STR("Variable " << name.name() << " already declared in current scope"), ast->location()};
            ctx.locals[name] = t;
        }

        Type * getVariable(Symbol name) {
            //i<e: loop until overflow
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
                throw TypeError{"Unable to type expression", child->location()};
            return result;
        }   

        /** We use this to track that the AST subtree we just typechecked had a valid return on all its control flow paths.
         *  This means we need to clear the flag in every statement and set the flag in return. Special handling is
         *  necessary at junctions such as after if or switch.
         */
        bool returned_ = false;

        std::vector<Context> contexts_; 


    }; // tiny::Typechecker


} // namespace tiny
