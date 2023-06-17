//
// Created by Mirek Škrabal on 17.06.2023.
//

#include "control_flow_tests.h"

std::vector<Test> control_flow_tests = {
    TEST("int main() { \
         int sum = 0; \
         for (int i = 0; i < 2; i = i + 1) {\
           sum = sum + 1; } \
         return sum; \
     }", 2),
    TEST("int main() { \
         int sum = 0; \
         for (int i = 0; i < 3; i = i + 1) {\
            if (i == 1)\
                continue;\
            sum = sum + 1; } \
         return sum; \
     }", 2),
    TEST("int main() { \
         int sum = 0; \
         for (int i = 0; i < 5; i = i + 1) {\
            if (i == 2)\
                break;\
            sum = sum + 1; } \
         return sum; \
     }", 2),
    TEST("int main() { \
         int sum = 0; \
         while (sum < 5) {\
            if (sum == 2)\
                break;\
            sum = sum + 1; } \
         return sum; \
     }", 2),
    TEST("int main() { \
         int sum = 0; \
         do {\
            sum = sum + 1;\
         }\
         while (sum < 1); \
         return sum; \
     }", 1),
    TEST("int main() { \
         int sum = 0; \
         do {\
            sum = sum + 1;\
         }\
         while (sum < 5); \
         return sum; \
     }", 5),
    TEST("int main(int a) { do { return 1; } while (a); }"),
    TEST("int main(int a) { if (a) { return 1; } else { return 2; }}"),
    TEST("int main(int a) { return 1; if (a) { return 2; }}"),
    TEST("int main() { if (1) {return 10;} else return 2; }", 10),
    TEST("int main() { if (0) return 10; else return 2; }", 2),
};

DEFINE_TEST_CATEGORY(control_flow_tests)
