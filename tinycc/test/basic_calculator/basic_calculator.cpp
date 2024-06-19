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
     }", 20),
    TEST("\
    int fibonacci(int n) { \
        if (n == 0) { \
            return 0;\
        } \
        else if (n == 1) { \
            return 1; \
        }\
        else {\
            int a = fibonacci(n-1);\
            int b = fibonacci(n-2);\
            return a + b; \
        }\
    } \
    int main() { \
        return fibonacci(8); \
     }", 21, true),
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
        int n = 0;\
        while (n < 10) {\
            n = n + 1;\
            if (5 < n ) {\
                break;\
            }\
        }\
         return n; \
     }", 6),
    TEST("\
    int main() { \
        int n = 0;\
        for (int i = 0; i < 10; i = i + 1) {\
            if (n > 5 ) {\
                continue;\
            }\
            n = n + 1;\
        }\
         return n; \
     }", 6),
    TEST("\
    int main() { \
        int n = 0;\
        for (int i = 0; i < 10; i = i + 1) {\
            if (n == 5 ) {\
                n = n + 6;\
                continue;\
            }\
            n = n + 1;\
        }\
         return n; \
     }", 15),

    TEST("\
    int main() { \
        int n = 0;\
        while (n < 10) {\
            int a = 1;\
            n = n + a;\
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
        int b = 5;\
        return 5;\
     }", 5),
    TEST("\
    int main() { \
        int a = 5;\
        if (a < 10) {\
            if (a < 5) {\
                return a;\
            }\
            int a = 4;\
            return 4;\
        }\
        else {\
            return 10;\
        }\
        int b = 5;\
        return 5;\
     }", 4),
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
    TEST("\
    int main() { \
        int a = 10;\
        if (a < 11) {\
            a = 15;\
        }\
        else {\
            a = 20;\
        }\
        return a;\
     }", 15),
    TEST("\
    int main() { \
        int a = 10;\
        a = a + a;\
        int b;\
        b = 2 * a;\
        if (a < 10) {\
            b = 5;\
            a = 15;\
        }\
        else {\
            a = 20;\
        }\
        a = a + b;\
        return a;\
     }", 60),
};

DEFINE_TEST_CATEGORY(basic_calculator_tests)
