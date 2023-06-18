//
// Created by Mirek Škrabal on 17.06.2023.
//

#pragma once

#include <cstdlib>
#include <tuple>
#include <map>
#include <string>

#define RUN_ALL_TEST_SUITES 0
#define RUN_MARKED_TESTS_ONLY 1


struct Test {
    char const * file;
    int line;
    char const * input;
    int64_t result = 0;
    bool testResult = false;
    char const * shouldError = nullptr;
    std::tuple<> args;
    bool marked = false;  // Add this field to your Test struct

    // Constructor for when only the test input is provided
    Test(char const * file, int line, char const * input):
            file{file}, line{line}, input{input} {
    }

    // Constructor for when both the test input and the expected result are provided
    Test(char const * file, int line, char const * input, int64_t result):
            file{file}, line{line}, input{input}, result{result}, testResult{true} {
    }

    // Constructor for when the test input, the expected result, and the `marked` flag are provided
    Test(char const * file, int line, char const * input, int64_t result, bool marked):
            file{file}, line{line}, input{input}, result{result}, testResult{true}, marked{marked} {
    }

    // Additional constructor for ERROR macro
    Test(char const * file, int line, char const * input, int64_t result, bool testResult, char const * shouldError):
            file{file}, line{line}, input{input}, result{result}, testResult{testResult}, shouldError{shouldError} {
    }

    template<typename... Args>
    Test(char const * file, int line, char const * input, int64_t result, Args&&... args):
            file{file}, line{line}, input{input}, result{result}, testResult{true}, shouldError{nullptr}, args{std::make_tuple(std::forward<Args>(args)...)} {
    }
};


#define ERROR(input, kind) Test{__FILE__, __LINE__, input, 0, false, # kind}
#define TEST_ARGS(input, result, ...) Test{__FILE__, __LINE__, input, result, __VA_ARGS__}

#define GET_MACRO(_1, _2, _3, NAME, ...) NAME
#define TEST(...) GET_MACRO(__VA_ARGS__, TEST_MARKED, TEST_RESULT, TEST_BASE)(__VA_ARGS__)

#define TEST_BASE(input) Test{__FILE__, __LINE__, input}
#define TEST_RESULT(input, result) Test{__FILE__, __LINE__, input, result}
#define TEST_MARKED(input, result, marked) Test{__FILE__, __LINE__, input, result, marked}



#define DEFINE_TEST_CATEGORY(category_name) \
    extern std::vector<Test> category_name; \
    namespace { \
        struct AutoRegister##category_name { \
            AutoRegister##category_name() { \
                testCategories[std::string(#category_name)] = category_name; \
            } \
        }; \
        AutoRegister##category_name autoRegister##category_name; \
    }

extern std::map<std::string, std::vector<Test>> testCategories;