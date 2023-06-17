//
// Created by Mirek Škrabal on 17.06.2023.
//

#include "function_tests.h"

std::vector<Test> function_tests = {
    TEST("\
    int foo() { \
        return 2; \
     } \
    int main() { \
         return foo(); \
     }", 2),

    TEST("\
    int id(int a) { \
        return a; \
     } \
    int main() { \
         return id(2); \
     }", 2),

    TEST("int fact(int n) { \
        if (n == 1) \
            return 1; \
        else \
            return fact(n - 1) * n; \
    } \
    int main() { \
        return fact(10); \
    }"),

    TEST("int sum(int a, int b) { return a + b; } int main() { return sum(5, 6); }"),
    TEST("int foo(int a, int b) { return a; } int main() { return foo(1, 2); }"),
    TEST("int main(int x) { return main(x); }"),
    TEST("int bar(int i) { if (i) return 10; else return 5; } int main() { return bar(5); }", 10),
    TEST("void bar(int * i) { *i = 10; } int main() { int i = 1; bar(&i); return i; }", 10),
};

DEFINE_TEST_CATEGORY(function_tests)
