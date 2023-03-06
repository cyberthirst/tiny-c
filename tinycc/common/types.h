#pragma once

#include <unordered_map>


#include "symbol.h"


namespace tiny {

    class SimpleType;

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


        static std::unordered_map<Symbol, Type*> initializeTypes();

        static std::unordered_map<Symbol, Type*> & types() {
            static std::unordered_map<Symbol, Type*> types{initializeTypes()};
            return types;
        }


    }; // tiny::Type


    class SimpleType : public Type {

    }; 


}