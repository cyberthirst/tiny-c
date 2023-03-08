#include "types.h"

namespace tiny {

    std::unordered_map<Symbol, Type*> Type::initializeTypes() {
        std::unordered_map<Symbol, Type*> result;
        result.insert(std::make_pair(Symbol::KwVoid, new SimpleType{}));
        result.insert(std::make_pair(Symbol::KwInt, new SimpleType{}));
        result.insert(std::make_pair(Symbol::KwChar, new SimpleType{}));
        result.insert(std::make_pair(Symbol::KwDouble, new SimpleType{}));

        return result;
    }


    PointerType * Type::getPointerTo(Type * base) {
        auto & t = pointerTypes();
        auto i = t.find(base);
        if (i == t.end())
            i = t.insert(std::make_pair(base, new PointerType{base})).first;
        return i->second;
    }

    FunctionType * Type::getFunction(std::vector<Type *> && sig) {
        // TODO

        return nullptr;        
    }



} // namespace tiny