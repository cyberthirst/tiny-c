//
// Created by Mirek Škrabal on 17.06.2023.
//

#include "cast_tests.h"

std::vector<Test> cast_tests = {
    TEST("int main(){ \
    int a = 4; \
    a = cast<int>(scan()); \
    print(cast<char>(a)); \
    return a; \
    }"),

    TEST("double add(double a, double b) { return a + b; } \
      int main() { int a = 3; double b = 4.5; double c = add(a, b); return (int)c; }"),

    TEST("int main() { for (int i = 0; i < 10; i++) { print(cast<char>(i + '0')); } return 666; }"),
    TEST("void main(int a) { a = cast<int>(4.0); }"),
    TEST("int* main() { return cast<int*>(0); }"),
};

DEFINE_TEST_CATEGORY(cast_tests)
