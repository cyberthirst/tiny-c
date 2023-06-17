//
// Created by Mirek Škrabal on 17.06.2023.
//

#include "array_tests.h"

std::vector<Test> array_tests = {
    TEST("int main() { int arr[5]; return arr[2]; }"),
    TEST("int main() { int arr[5]; return arr[10]; }"),
};

DEFINE_TEST_CATEGORY(array_tests)
