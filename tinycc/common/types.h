#pragma once

#include <unordered_map>


#include "symbol.h"


namespace tiny {

    class SimpleType;
    class PointerType;
    class FunctionType;

    /** Basic type class. 
     */
    class Type {
    public:

        virtual ~Type() = default;



        static Type * getVoid() { return getType(Symbol::KwVoid); }
        static Type * getInt() { return getType(Symbol::KwInt); }
        static Type * getDouble() {  return getType(Symbol::KwDouble); }
        static Type * getChar() {  return getType(Symbol::KwChar); }

        /** Returns the type, or nullptr if not found. 
         */
        static Type * getType(Symbol name) {
            auto & t = types();
            auto i = t.find(name);
            if (i == t.end())
                return nullptr;
            return i->second;
        }

        static PointerType * getPointerTo(Type * base);

        static FunctionType * getFunction(std::vector<Type *> && sig);


        static std::unordered_map<Symbol, Type*> initializeTypes();

        static std::unordered_map<Symbol, Type*> & types() {
            static std::unordered_map<Symbol, Type*> types{initializeTypes()};
            return types;
        }

        static std::unordered_map<Type *, PointerType*> & pointerTypes() {
            static std::unordered_map<Type *, PointerType*> types;
            return types;
        }


    }; // tiny::Type


    class SimpleType : public Type {

    }; 

    class PointerType : public Type {
    public:

        Type * base() const { return base_; }

    private:    
        friend class Type;

        PointerType(Type * base):base_{base} {}

        Type * base_;
    };

    class FunctionType : public Type {
    public:
        Type * returnType() const { return sig_[0]; }
        Type * argType(size_t i) const { return sig_[i + 1]; }
        size_t numArgs() const { return sig_.size() - 1; }

    private:
        friend Type;

        FunctionType(std::vector<Type *> && sig):sig_{std::move(sig)} {}

        std::vector<Type *> sig_;
    }; 

}