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
    char const * shouldError = nullptr;

    Test(char const * file, int line, char const * input):
        file{file}, line{line}, input{input} {
    }

    Test(char const * input, int result):
        file{file}, line{line}, input{input}, result{result}, testResult{true} {
    }

    Test(char const * file, int line, char const * input, int result, bool testResult, char const * shouldError):
        file{file}, line{line}, input{input}, result{result}, testResult{testResult}, shouldError{shouldError} {
    }
};

#define TEST(...) Test{__FILE__, __LINE__, __VA_ARGS__}
#define ERROR(input, kind) Test{__FILE__, __LINE__, input, 0, false, # kind} 

Test tests[] = {
    TEST("void main() {}"),
    // Tests that exercise the parser, lexer and a typechecker
    ERROR("void main() { return 1; }", TypeError), 
    TEST("int main() { return 1; }"),
    TEST("void main() { return; }"),
    TEST("void main() { break; }"),
    TEST("int* main() { return cast<int*>(0); }"),
    TEST("int main() { int a; return a; }"),
    TEST("double main() { double a; return a; }"),
    TEST("int main() { if (1) return 1; else return 2; }"),
    ERROR("int main() { if (1) return 1; else return 2.0; }", TypeError),
    ERROR("void main() { a = 5; }", TypeError),
    TEST("void main() { int a; a = 7; }"),
    ERROR("void main() { int a; a = 7.3; }", TypeError),
    TEST("int* main() { int a; return & a; }"),
    ERROR("void* main() { int a; return & a; }", TypeError),
    TEST("int main() { int * a; return *a; }"),
    TEST("int main(int a) { return a; }"),
    TEST("int foo(int a, int b) { return a; } int main() { return foo(1, 2); }"),
    ERROR("int foo(int a, int b) { return a; } int main() { return foo(1, 'a'); }", TypeError),
    ERROR("int foo(int a, int b) { return a; } int main() { return foo(1); }", TypeError),
    ERROR("int foo(int a, int b) { return a; } int main() { return bar(1, 2); }", TypeError),
    TEST("int main(int x) { return main(x); }"),
    TEST("int a = 56; int main() { return a; }"),
    TEST("double b = 6.7; int main() { int a = 1; return a; }"),
    TEST("struct Foo { }; void main(Foo x) {}"),
    ERROR("struct Foo; void main(Foo x) {}", TypeError),
    TEST("struct Foo; struct Foo { int i; }; void main(Foo x) {}"),
    ERROR("struct Foo; struct Foo { int i; }; struct Foo { int i; }; void main(Foo x) {}", TypeError),
    TEST("int main(int argc) { return argc++; }"),
    TEST("double main(double argc) { return argc++; }"),
    TEST("char main(char argc) { return argc++; }"),
    TEST("int * main(int * argc) { return argc++; }"),
    ERROR("struct Foo {}; Foo main(Foo argc) { return argc++; }", TypeError),
    TEST("int main(int argc) { return ++argc; }"),
    TEST("double main(double argc) { return ++argc; }"),
    TEST("char main(char argc) { return ++argc; }"),
    TEST("int * main(int * argc) { return ++argc; }"),
    ERROR("struct Foo {}; Foo main(Foo argc) { return ++argc; }", TypeError),
    TEST("int main(int i) { return +i; }"),
    TEST("int main(int i) { return -i; }"),
    TEST("int main(int i) { return ~i; }"),
    TEST("int main(int i) { return !i; }"),
    TEST("int main(int i) { return ++i; }"),
    TEST("int main(int i) { return --i; }"),
    TEST("double main(double i) { return +i; }"),
    TEST("double main(double i) { return -i; }"),
    ERROR("double main(double i) { return ~i; }", TypeError),
    TEST("int main(double i) { return !i; }"),
    TEST("double main(double i) { return ++i; }"),
    TEST("double main(double i) { return --i; }"),
    TEST("char main(char i) { return +i; }"),
    TEST("char main(char i) { return -i; }"),
    TEST("char main(char i) { return ~i; }"),
    TEST("int main(char i) { return !i; }"),
    TEST("char main(char i) { return ++i; }"),
    TEST("char main(char i) { return --i; }"),
    ERROR("int * main(int * i) { return +i; }", TypeError),
    ERROR("int * main(int * i) { return -i; }", TypeError),
    ERROR("int * main(int * i) { return ~i; }", TypeError),
    TEST("int main(int * i) { return !i; }"),
    TEST("int * main(int * i) { return ++i; }"),
    TEST("int * main(int * i) { return --i; }"),
    ERROR("struct Foo {}; Foo main(Foo i) { return +i; }", TypeError),
    ERROR("struct Foo {}; Foo main(Foo i) { return -i; }", TypeError),
    ERROR("struct Foo {}; Foo main(Foo i) { return ~i; }", TypeError),
    ERROR("struct Foo {}; Foo main(Foo i) { return !i; }", TypeError),
    ERROR("struct Foo {}; Foo main(Foo i) { return ++i; }", TypeError),
    ERROR("struct Foo {}; Foo main(Foo i) { return --i; }", TypeError),
    TEST("int main(int a, int b) { return a + b; }"),
    TEST("int main(int a, int b) { return a - b; }"),
    TEST("int main(int a, int b) { return a * b; }"),
    TEST("int main(int a, int b) { return a / b; }"),
    TEST("int main(int a, int b) { return a % b; }"),
    TEST("int main(int a, int b) { return a << b; }"),
    TEST("int main(int a, int b) { return a >> b; }"),
    TEST("int main(int a, int b) { return a > b; }"),
    TEST("int main(int a, int b) { return a < b; }"),
    TEST("int main(int a, int b) { return a >= b; }"),
    TEST("int main(int a, int b) { return a <= b; }"),
    TEST("int main(int a, int b) { return a == b; }"),
    TEST("int main(int a, int b) { return a != b; }"),
    TEST("int main(int a, int b) { return a & b; }"),
    TEST("int main(int a, int b) { return a | b; }"),
    TEST("int main(int a, int b) { return a && b; }"),
    TEST("int main(int a, int b) { return a || b; }"),
    TEST("int main(double a, double b) { return a > b; }"),
    TEST("int main(double a, int b) { return a > b; }"),
    TEST("int main(int a, double b) { return a > b; }"),
    TEST("int main(int * a, int * b) { return a > b; }"),
    TEST("double main(int a, double b) { return a + b; }"),
    TEST("int * main(int a, int * b) { return b + a; }"),
    TEST("void main(int a) { a = cast<int>(4.0); }"),
    ERROR("void main() { 5 = 6; }", TypeError),
    TEST("void main(double * a) { *a = 6.0; }"),
    TEST("void main(int * a) { a = 678; }"),
    ERROR("void main(int a) { & a = 678; }", TypeError),
    ERROR("int main() {}", TypeError),
    ERROR("int main(int a) { if (a) { return 1; }}", TypeError),
    TEST("int main(int a) { if (a) { return 1; } else { return 2; }}"),
    ERROR("int main(int a) { while (a < 10) { return 1; }}", TypeError), 
    TEST("int main(int a) { do { return 1; } while (a); }"), 
    TEST("int main(int a) { return 1; if (a) { return 2; }}"),

    TEST("void main() { scan(); }"),
    TEST("void main() { print('a'); }"),
    // scan and print and intrinsic functions, i.e. they are handled specially by the compiler. This allows us to deal with them in different ways than normal functions, such as we can overload based on the print argument, which is something tinyC does not support for functions. However, our handling of intrinsics in the typechecker is to create a "fake" functions for them so this does not work atm. A good extra HW is to make this test work:
    ERROR("void main() { print(67); }", TypeError),

    // some full program tests
    TEST("int main(){ \
        int a = 4; \
        a = cast<int>(scan()); \
        print(cast<char>(a)); \
        return a; \
    }"),
    TEST("int fact(int n) { \
        if (n == 1) \
            return 1; \
        else \
            return fact(n - 1) * n; \
    } \
    int main() { \
        return fact(10); \
    }"),

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
