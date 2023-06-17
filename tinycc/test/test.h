//
// Created by Mirek Škrabal on 17.06.2023.
//

#pragma once

#include <cstdlib>
#include <tuple>
#include <map>
#include <string>

#define RUN_ALL 0

struct Test {
    char const * file;
    int line;
    char const * input;
    int64_t result = 0;
    bool testResult = false;
    char const * shouldError = nullptr;
    std::tuple<> args;

    Test(char const * file, int line, char const * input):
            file{file}, line{line}, input{input} {
    }

    Test(char const * file, int line, char const * input, int64_t result):
            file{file}, line{line}, input{input}, result{result}, testResult{true} {
    }

    Test(char const * file, int line, char const * input, int result, bool testResult, char const * shouldError):
            file{file}, line{line}, input{input}, result{result}, testResult{testResult}, shouldError{shouldError} {
    }

    template<typename... Args>
    Test(char const * file, int line, char const * input, int64_t result, Args&&... args):
            file{file}, line{line}, input{input}, result{result}, testResult{true}, shouldError{nullptr}, args{std::make_tuple(std::forward<Args>(args)...)} {
    }
};

#define TEST(...) Test{__FILE__, __LINE__, __VA_ARGS__}
#define TEST_ARGS(input, result, ...) Test{__FILE__, __LINE__, input, result, __VA_ARGS__}
#define ERROR(input, kind) Test{__FILE__, __LINE__, input, 0, false, # kind}

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