//
// Created by Mirek Škrabal on 17.06.2023.
//

#include "pointer_tests.h"

std::vector<Test> pointer_tests = {
    TEST("int main() { \
         int sum = 0; \
         int *p = &sum;\
         *p = 2;\
         return sum; \
     }", 2),
    TEST("int main() { \
         int sum = 0; \
         int *p = &sum;\
         *p = 2;\
         return *p; \
     }", 2),
    TEST("void swap(int *a, int *b) { int temp = *a; *a = *b; *b = temp; } int main() { int x = 10; int y = 20; swap(&x, &y); return x + y; }"),
    ERROR("int main() { int a; a = &5; return *a; }", TypeError),
    TEST("int* main() { int a; return & a; }"),
    TEST("int main() { int * a; return *a; }"),
    TEST("void main(double * a) { *a = 6.0; }"),
    TEST("void main(int * a) { a = 678; }"),
};