//
// Created by Mirek Škrabal on 17.06.2023.
//

#include "arithmetic_tests.h"

std::vector<Test> arithmetic_tests = {
    TEST("int main() { return 2 * 2; }", 4),
    TEST("int main() { return 4 / 2; }", 2),
    TEST("int main() { return 2 + 2; }", 4),
    TEST("int main() { return 4 - 2; }", 2),
    TEST("int main() { double a = 2.5; int b = 3; char c = 'A'; return a * (b - c); }"),
    TEST("int main() { int a = 2; double b = 3.5; char c = 'A'; return a * (int)b - c; }"),
    TEST("int main() { int a = 3; double b = 4.5; return a && b; }"),
    TEST("int main() { int a = 5; double b = 6.5; char c = 'A'; return a * (int)b + c; }"),
    TEST("int main() { double a = 5.5; char b = 'A'; return a & b; }"),
    TEST("int main() { int a = 2; double b = 3.5; return a % b; }"),
    TEST("int main() { double a = 3.5; char b = 'A'; return a + b * 2.0; }"),
    TEST("char getChar() { return 'A'; } \
          int main() { int a = 5; char b = getChar(); int c = 2; return a * (b - c); }"),
};

DEFINE_TEST_CATEGORY(arithmetic_tests)

