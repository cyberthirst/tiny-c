#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <map>

#include "common/options.h"
#include "common/colors.h"
#include "frontend/parser.h"
#include "frontend/typechecker.h"
#include "optimizer/ast_to_il.h"
#include "optimizer/il_interpreter.h"
#include "backend/il_to_t86.h"
#include "optimizer/optimizer.h"

//tests
#include "test/arithmetic/arithmetic_tests.h"
#include "test/control_flow/control_flow_tests.h"
#include "test/arrays/array_tests.h"
#include "test/casts/cast_tests.h"
#include "test/io/io_tests.h"
#include "test/pointers/pointer_tests.h"
#include "test/functions/function_tests.h"
#include "test/struct/struct_tests.h"

using namespace tiny;
using namespace colors;

std::map<std::string, std::vector<Test>> testCategories;

struct TestResult {
    size_t total_tests = 0;
    size_t total_fails = 0;
    size_t typechecks = 0;
    size_t typecheck_fails = 0;
};

bool testIRProgram(Program const & p, Test const * test) {
    if (test->marked)
        std::cout << ColorPrinter::colorize(p) << std::endl;
    int64_t result = ILInterpreter::run(p);
    if (result != test->result) {
        std::cerr << "ERROR: expected " << test->result << ", got " << result << color::reset << std::endl;
        return false;
    }
    return true;
}

bool compile(std::string const & contents, Test const * test, TestResult *result) {
    try {
        // parse
        std::unique_ptr<AST> ast = (test == nullptr) ? Parser::parseFile(contents) : Parser::parse(contents);
        if (Options::verboseAST)
            std::cout << ColorPrinter::colorize(*ast) << std::endl;
        // typecheck
        Typechecker::checkProgram(ast);
        if (test && !test->testResult)
            ++result->typechecks;
        Program p = ASTToILTranslator::translateProgram(ast);
        if (test && test->testResult && Options::testIR) {
            if (!testIRProgram(p, test))
                return false;
        }
        // optimize
        Optimizer::optimize(p);
        if (!testIRProgram(p, test))
            return false;
        // translate to target
        t86::Program t86 = T86CodeGen::translateProgram(p);
        // run on t86, or output and verify?
        return (test == nullptr) || ! (test->shouldError);
    } catch (SourceError const & e) {
        if ((test != nullptr) && test->shouldError == e.kind())
            return true;
        if (e.kind() == "TypeError")
            ++result->typecheck_fails;
        std::cerr << color::red << "ERROR: " << color::reset << e << std::endl;
    } catch (std::exception const &e) {
        std::cerr << color::red << "ERROR: " << color::reset << e.what() << std::endl;
    } catch (...) {
        std::cerr << color::red << "UNKNOWN ERROR. " << color::reset << std::endl;
    }
    return false;
}

void RunSelectedTestSuite(const std::string& suiteName) {
    auto it = testCategories.find(suiteName);
    if (it == testCategories.end()) {
        std::cerr << "Test suite '" << suiteName << "' not found." << std::endl;
        return;
    }

    TestResult *result = new  TestResult();
    result->total_tests = 0;
    result->total_fails = 0;

    std::cout << "Running tests in category: " << color::blue << suiteName << color::reset << std::endl;

    for (auto const & t : it->second) {
        if (RUN_MARKED_TESTS_ONLY && !t.marked)
            continue;
        else
            ++result->total_tests;
        if (! compile(t.input, & t, result)) {
            std::cout << color::red << t.file << ":" << t.line << ": Test failed." << color::reset << std::endl;
            std::cout << "    " << t.input << std::endl;
            ++result->total_fails;
            if (Options::exitAfterFailure)
                break;
        }
    }
    if (result->total_fails > 0) {
        std::cout << color::red << "All: " << result->total_fails << "/" << result->total_tests << " failed." << color::reset << std::endl;
        std::cout << color::red << "Typecheck: " << result->typecheck_fails << "/" << result->typechecks << " failed." << color::reset << std::endl;
    } else {
        std::cout << color::green << "PASS. All " << result->total_tests << " tests passed, " <<  result->typechecks << " were typecker tests." << color::reset << std::endl;
    }
}

void RunAllTestSuites() {
    for (const auto& [suiteName, tests] : testCategories) {
        RunSelectedTestSuite(suiteName);
    }
}

int main(int argc, char * argv []) {
    initializeTerminal();
    // parse arguments
    char const * filename = nullptr;
    if (Options::parseArgs(argc, argv, filename) == EXIT_FAILURE)
        return EXIT_FAILURE;

    if (filename == nullptr) {
        if (RUN_ALL_TEST_SUITES) {
            RunAllTestSuites();
        } else {
            RunSelectedTestSuite("basic_calculator_tests");
            //RunSelectedTestSuite("function_tests");
        }
    } else {
        std::cout << "Compiling file " << filename << "..." << std::endl;
        if (! compile(filename, /* test */nullptr, nullptr))
            return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
