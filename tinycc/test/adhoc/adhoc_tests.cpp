//
// Created by Mirek Škrabal on 17.06.2023.
//

#include "adhoc_tests.h"

std::vector<Test> adhoc_tests{
        TEST("\
    int main() { \
        int a = 5;\
        int b = 5;\
        if (a < 5) {\
            int a = 5;\
            {\
                int a = 10;\
            }\
            {\
                int a = 10;\
            }\
        }\
        else {\
            int b = 5;\
        }\
        return a * b; \
     }", 25, true),
};

DEFINE_TEST_CATEGORY(adhoc_tests)
