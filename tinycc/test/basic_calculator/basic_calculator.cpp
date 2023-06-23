//
// Created by Mirek Škrabal on 18.06.2023.
//

#include "basic_calculator.h"

std::vector<Test> basic_calculator_tests = {
    TEST("\
    int add_one(int a) { \
        return a + 1; \
     } \
    int main() { \
        return add_one(1); \
     }", 2),
    TEST("\
    int add_n(int a, int n) { \
        while (0 < n) {\
            a = a + 1;\
            n = n - 1;\
        }\
        return a; \
     } \
    int main() { \
         return add_n(0, 20); \
     }", 20, true),
    TEST("\
    int main() { \
        int n = 0;\
        while (n < 10) {\
            n = n + 1;\
        }\
         return n; \
     }", 10),
    TEST("\
    int main() { \
        return 1 + 10; \
     }", 11),
    TEST("\
    int main() { \
        return 1 - 10; \
     }", -9),
    TEST("\
    int main() { \
        int a = 5;\
        int b = 5;\
        return a * b; \
     }", 25),
    TEST("\
    int main() { \
        int a = 5;\
        int b = 5;\
        return a + b; \
     }", 10),
    TEST("\
    int main() { \
        int a = 5;\
        int b = 5 + 5 - 2;\
        return a * b; \
     }", 40),
    TEST("\
    int main() { \
        int a = 5;\
        if (a < 10) {\
            return 5;\
        }\
        else {\
            return 10;\
        }\
     }", 5),
    TEST("\
    int main() { \
        int a = 10;\
        if (a < 10) {\
            a = 15;\
        }\
        else {\
            a = 20;\
        }\
        return a;\
     }", 20),
};

DEFINE_TEST_CATEGORY(basic_calculator_tests)
