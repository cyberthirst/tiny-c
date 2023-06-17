//
// Created by Mirek Škrabal on 17.06.2023.
//

#include "typedef_tests.h"

std::vector<Test> typedef_tests = {
    TEST("typedef int (*func_ptr_t)(int, int); int add(int x, int y) { return x + y; } int main() { func_ptr_t ptr = add; return ptr(5, 6); }"),
    TEST("typedef void (*func_ptr_t)(int); void foo(int x) { print(cast<char>(x + 1)); } int main() { func_ptr_t ptr = foo; ptr(5); return 0; }"),
    TEST("typedef int (*func_ptr_t)(int); int foo(int x) { return x + 1; } int apply(func_ptr_t f, int x) { return f(x); } int main() { return apply(foo, 5); }"),
    // Assigning a function with incompatible signature to a function pointer is a type error
    // Passing a function with incompatible signature to a function expecting a function pointer is a type error
    ERROR("typedef int (*func_ptr_t)(int); int foo(int x, int y) { return x + y; } int apply(func_ptr_t f, int x) { return f(x); } int main() { return apply(foo, 5); }",
          TypeError),
};