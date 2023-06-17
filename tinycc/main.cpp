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

bool compile(std::string const & contents, Test const * test) {
    try {
        // parse
        std::unique_ptr<AST> ast = (test == nullptr) ? Parser::parseFile(contents) : Parser::parse(contents);
        if (Options::verboseAST)
            std::cout << ColorPrinter::colorize(*ast) << std::endl;
        // typecheck
        Typechecker::checkProgram(ast);
        // translate to IR
        Program p = ASTToILTranslator::translateProgram(ast);
        if (Options::verboseIL)
            std::cout << ColorPrinter::colorize(p) << std::endl;
        if (test && test->testResult && Options::testIR) {
            int64_t result = ILInterpreter::run(p);
            if (result != test->result) {
                std::cerr << "ERROR: expected " << test->result << ", got " << result << color::reset << std::endl;
                return false;
            }
        }
        // optimize
        // TODO
        // translate to target
        // TODO
        // run on t86, or output and verify?
        return (test == nullptr) || ! (test->shouldError);
    } catch (SourceError const & e) {
        if ((test != nullptr) && test->shouldError == e.kind())
            return true;
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

    size_t ntests = it->second.size();
    size_t fails = 0;

    std::cout << "Running " << ntests << " tests in category: " << suiteName << std::endl;
    for (auto const & t : it->second) {
        if (! compile(t.input, & t)) {
            std::cout << color::red << t.file << ":" << t.line << ": Test failed." << color::reset << std::endl;
            std::cout << "    " << t.input << std::endl;
            ++fails;
            if (Options::exitAfterFailure)
                break;
        }
    }
    std::cout << "Finished running tests in category: " << suiteName << std::endl;
    if (fails > 0) {
        std::cout << color::red << "FAIL. Total " << fails << " tests out of " << ntests << " failed." << color::reset << std::endl;
    } else {
        std::cout << color::green << "PASS. All " << ntests << " tests passed." << color::reset << std::endl;
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
        if (RUN_ALL) {
            RunAllTestSuites();
        } else {
            RunSelectedTestSuite("arithmetic_tests");
            //RunSelectedTestSuite("control_flow_tests");
            //RunSelectedTestSuite("function_tests");
        }
    } else {
        std::cout << "Compiling file " << filename << "..." << std::endl;
        if (! compile(filename, /* test */nullptr))
            return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
