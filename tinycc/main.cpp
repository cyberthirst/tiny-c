#include <cstdlib>
#include <cstring>
#include <iostream>

#include "common/colors.h"
#include "frontend/parser.h"
#include "frontend/typechecker.h"

using namespace tiny;
using namespace colors;

struct Test {
    char const * file;
    int line;
    char const * input;
    int result = 0;
    bool testResult = false;
    bool shouldError = false;

    Test(char const * file, int line, char const * input):
        file{file}, line{line}, input{input} {
    }

    Test(char const * input, int result):
        file{file}, line{line}, input{input}, result{result}, testResult{true} {
    }

    Test(char const * file, int line, char const * input, int result, bool testResult, bool shouldError):
        file{file}, line{line}, input{input}, result{result}, testResult{testResult}, shouldError{shouldError} {
    }
};

#define TEST(...) Test{__FILE__, __LINE__, __VA_ARGS__}
#define ERROR(input) Test{__FILE__, __LINE__, input, 0, false, true} 

Test tests[] = {
    TEST("void main() {}"),
    ERROR("foo haha bubu"),
    TEST("void main() { 4; }"),
};

bool verbose = false;

bool compile(std::string const & contents, Test const * test) {
    try {
        // parse
        std::unique_ptr<AST> ast = (test == nullptr) ? Parser::parseFile(contents) : Parser::parse(contents);
        if (verbose)
            std::cout << ColorPrinter::colorize(*ast) << std::endl;
        // typecheck
        Typechecker::checkProgram(ast);
        // translate to IR
        // TODO
        // optimize
        // TODO
        // translate to target
        // TODO
        // run on t86, or output and verify?
       return true;
    } catch (std::exception const &e) {
        if (test != nullptr && test->shouldError)
            return true;
        std::cerr << color::red << "ERROR: " << color::reset << e.what() << std::endl;
    } catch (...) {
        if (test != nullptr && test->shouldError)
            return true;
        std::cerr << color::red << "UNKNOWN ERROR. " << color::reset << std::endl;
    }
    if (test != nullptr && test->shouldError)
        return true;
    else 
        return false;
}

int main(int argc, char * argv []) {
    initializeTerminal();
    std::cout << color::gray << "The one and only Tiny-C brought to you by NI-GEN" << color::reset << std::endl;
    // parse arguments
    char const * filename = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        } else if (filename == nullptr) {
            filename = argv[i];
        } else {
            std::cerr << color::red << "ERROR: Invalid argument " << argv[i] << color::reset << std::endl;
            return EXIT_FAILURE;
        }
    }
    if (filename == nullptr) {
        size_t ntests = sizeof(tests) / sizeof(Test);
        size_t fails = 0;
        std::cout << "Running " << ntests << " tests..." << std::endl;
        for (auto const & t : tests) {
            if (! compile(t.input, & t)) {
                std::cout << color::red << t.file << ":" << t.line << ": Test failed." << color::reset << std::endl;
                std::cout << "    " << t.input << std::endl;
                ++fails;
            }
        }
        if (fails > 0) {
            std::cout << color::red << "FAIL. Total " << fails << " tests out of " << ntests << " failed." << color::reset << std::endl;
            return EXIT_FAILURE;
        } else {
            std::cout << color::green << "PASS. All " << ntests << " tests passed." << color::reset << std::endl;
        }
    } else {
        std::cout << "Compiling file " << filename << "..." << std::endl;
        if (! compile(filename, /* test */nullptr))
            return EXIT_FAILURE;
    }
   return EXIT_SUCCESS;
}