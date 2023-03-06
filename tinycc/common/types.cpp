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


} // namespace tiny