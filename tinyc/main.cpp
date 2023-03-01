#include <cstdlib>
#include <cstring>
#include <iostream>

#include "colors.h"
#include "frontend/parser.h"

using namespace tiny;
using namespace colors;

char const * tests[] = {
    "void main() {}", // 0
};




bool verbose = false;

bool compile(std::string const & contents, bool isFilename = false) {
    try {
        // parse
        std::unique_ptr<AST> ast = isFilename ? Parser::parseFile(contents) : Parser::parse(contents);
        if (verbose)
            std::cout << ColorPrinter::colorize(*ast) << std::endl;
        // typecheck
        // TODO
        // translate to IR
        // TODO
        // optimize
        // TODO
        // translate to target
        // TODO
        // run on t86, or output and verify?
       return true;
    } catch (std::exception const &e) {
        std::cerr << color::red << "ERROR: " << color::reset << e.what() << std::endl;
    } catch (...) {
        std::cerr << color::red << "UNKNOWN ERROR. " << color::reset << std::endl;
    }
    return false;
}




int main(int argc, char *argv[])
{
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
        size_t ntests = sizeof(tests) / sizeof(char const *);
        size_t fails = 0;
        std::cout << "Running " << ntests << " tests..." << std::endl;
        for (size_t i = 0; i < ntests; ++i)
            if (! compile(tests[i])) {
                std::cout << color::red << " Test " << i << " failed." << color::reset << std::endl;
                ++fails;
            }
        if (fails > 0) {
            std::cout << color::red << "FAIL. Total " << fails << " tests out of " << ntests << " failed." << color::reset << std::endl;
            return EXIT_FAILURE;
        } else {
            std::cout << color::green << "PASS. All " << ntests << " tests passed." << color::reset << std::endl;
        }
    } else {
        std::cout << "Compiling file " << filename << "..." << std::endl;
        if (! compile(filename, /*isFilename */true))
            return EXIT_FAILURE;
    }
   return EXIT_SUCCESS;
}