#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

#include "common/options.h"
#include "common/colors.h"
#include "frontend/parser.h"
#include "frontend/typechecker.h"
#include "optimizer/ast_to_il.h"
#include "optimizer/il_interpreter.h"

using namespace tiny;
using namespace colors;

struct Test {
    char const * file;
    int line;
    char const * input;
    int64_t result = 0;
    bool testResult = false;
    char const * shouldError = nullptr;

    Test(char const * file, int line, char const * input):
        file{file}, line{line}, input{input} {
    }

    Test(char const * file, int line, char const * input, int64_t result):
        file{file}, line{line}, input{input}, result{result}, testResult{true} {
    }

    Test(char const * file, int line, char const * input, int result, bool testResult, char const * shouldError):
        file{file}, line{line}, input{input}, result{result}, testResult{testResult}, shouldError{shouldError} {
    }
};

#define TEST(...) Test{__FILE__, __LINE__, __VA_ARGS__}
#define ERROR(input, kind) Test{__FILE__, __LINE__, input, 0, false, # kind} 

Test tests[] = {
    TEST("int main() { return 1; }", 1),
    TEST("int main() { if (1) return 10; else return 2; }", 10),
    TEST("int main() { if (0) return 10; else return 2; }", 2),
    TEST("int main() { int i = 1; return i; }", 1),
    TEST("int bar(int i) { return i; } int main() { return bar(5); }", 5),
    TEST("int bar(int i) { if (i) return 10; else return 5; } int main() { return bar(5); }", 10),
    TEST("void bar(int * i) { *i = 10; } int main() { int i = 1; bar(&i); return i; }", 10),

/*    TEST("void main(int a, int b) {}"),
    TEST("void main(int a, int b) { 1 * 2; }"),
    TEST("void main(int a, int b) { a * b; }"),
    TEST("void main(int a, int b) { a = 3; }"),
    TEST("void main(int a, int b) { a = b; }"),
*/
    // Tests that exercise the parser, lexer and a typechecker
    TEST("void main() {}"),
    ERROR("void main() { return 1; }", TypeError), 
    TEST("void main() { 1 * 2; }"),
    TEST("void main() { int a; }"),
    TEST("int main() { return 1; }"),
    TEST("void main() { return; }"),
    //ERROR("void main() { break; }", ParserError),
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
    TEST("int* main() { return cast<int*>(0); }"),
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
    //MY TESTS
    TEST("int main() { for (int i = 0; i < 10; i++) { print(cast<char>(i + '0')); } return 666; }"),
    TEST("int sum(int a, int b) { return a + b; } int main() { return sum(5, 6); }"),
    /*TEST("int main() { int arr[5]; return arr[2]; }"),
    TEST("void swap(int *a, int *b) { int temp = *a; *a = *b; *b = temp; } int main() { int x = 10; int y = 20; swap(&x, &y); return x + y; }"),
    // Assigning a float to an int variable is a type error
    ERROR("int main() { int a = 2.5; return a; }", TypeError),
    // Assigning a char to an int variable is a type error
    ERROR("int main() { int a = 'a'; return a + 5; }", TypeError),
    // Trying to assign the address of a literal integer to an int variable is a type error
    ERROR("int main() { int a; a = &5; return *a; }", TypeError),
    // TODO Accessing an array out of bounds is undefined behavior, so it should be a type error
    // we actually treat arrays as ptrs so this passes
    TEST("int main() { int arr[5]; return arr[10]; }"),*/
    //TEST("typedef int (*func_ptr_t)(int); int foo(int x) { return x + 1; } int main() { func_ptr_t ptr = foo; return ptr(5); }"),
    TEST("typedef int (*func_ptr_t)(int); int foo(int x) { return x + 1; } int main() { func_ptr_t ptr; ptr = foo; return ptr(5); }"),
    /*TEST("typedef int (*func_ptr_t)(int, int); int add(int x, int y) { return x + y; } int main() { func_ptr_t ptr = add; return ptr(5, 6); }"),
    TEST("typedef void (*func_ptr_t)(int); void foo(int x) { print(cast<char>(x + 1)); } int main() { func_ptr_t ptr = foo; ptr(5); return 0; }"),
    TEST("typedef int (*func_ptr_t)(int); int foo(int x) { return x + 1; } int apply(func_ptr_t f, int x) { return f(x); } int main() { return apply(foo, 5); }"),
    // Assigning a function with incompatible signature to a function pointer is a type error
    ERROR("typedef int (*func_ptr_t)(int); int foo(int x, int y) { return x + y; } int main() { func_ptr_t ptr = foo; return 0; }", TypeError),
    // Passing a function with incompatible signature to a function expecting a function pointer is a type error
    ERROR("typedef int (*func_ptr_t)(int); int foo(int x, int y) { return x + y; } int apply(func_ptr_t f, int x) { return f(x); } int main() { return apply(foo, 5); }", TypeError),
    // Assigning an integer value to a function pointer is a type error
    ERROR("typedef int (*func_ptr_t)(int); int main() { func_ptr_t ptr = 42; return 0; }", TypeError),
    // Calling a function pointer with the wrong number of arguments is a type error
    ERROR("typedef int (*func_ptr_t)(int); int foo(int x) { return x + 1; } int main() { func_ptr_t ptr = foo; return ptr(5, 6); }", TypeError),
    // Calling a function pointer with an incompatible argument type is a type error
    ERROR("typedef int (*func_ptr_t)(int); int foo(int x) { return x + 1; } int main() { func_ptr_t ptr = foo; return ptr(5.5); }", TypeError),
    TEST("struct Point { int x; int y; }; int main() { struct Point p; p.x = 5; p.y = 6; return p.x + p.y; }"),
    TEST("struct Point { int x; int y; }; struct Line { struct Point p1; struct Point p2; }; int main() { struct Line l; l.p1.x = 1; l.p1.y = 2; l.p2.x = 3; l.p2.y = 4; return l.p1.x + l.p1.y + l.p2.x + l.p2.y; }"),
    TEST("struct Point { int x; int y; }; struct Box { struct Point topLeft; struct Point bottomRight; }; int main() { struct Box b; b.topLeft.x = 1; b.topLeft.y = 2; b.bottomRight.x = 5; b.bottomRight.y = 6; return b.topLeft.x + b.topLeft.y + b.bottomRight.x + b.bottomRight.y; }"),
    TEST("struct Node; struct Node { int value; struct Node *next; }; int main() { struct Node n1, n2; n1.value = 5; n1.next = &n2; n2.value = 6; n2.next = 0; return n1.value + n1.next->value; }"),
    // Redeclaring a struct field with the same name is a type error
    ERROR("struct Point { int x; int y; int x; }; int main() { return 0; }", TypeError),
    // Accessing an undeclared struct field is a type error
    ERROR("struct Point { int x; int y; }; int main() { struct Point p; p.z = 5; return 0; }", TypeError),
    // Assigning an incompatible type to a struct field is a type error
    ERROR("struct Point { int x; int y; }; int main() { struct Point p; p.x = 5; p.y = 6.5; return 0; }", TypeError),
    // Using an incomplete struct definition is a type error
    ERROR("struct Point; int main() { struct Point p; p.x = 5; p.y = 6; return p.x + p.y; }", TypeError),
    // Using an undefined struct is a type error
    ERROR("int main() { struct UndefinedPoint p; p.x = 5; p.y = 6; return p.x + p.y; }", TypeError),
    TEST("double add(double a, double b) { return a + b; } \  
          int main() { int a = 3; double b = 4.5; double c = add(a, b); return (int)c; }"),
    TEST("char getChar() { return 'A'; } \
          int main() { int a = 5; char b = getChar(); int c = 2; return a * (b - c); }"),
    TEST("int getInt() { return 3; } \ 
          int main() { double a = 3.5; int b = getInt(); double c = a * b; return (int)c; }"),
    TEST("int main() { int a = 5; double b = 6.5; char c = 'A'; return a * (int)b + c; }"),
    TEST("int main() { int a = 2; double b = 3.5; char c = 'A'; return a * (int)b - c; }")
    TEST("int main() { int a = 3; double b = 4.5; return a && b; }"),
    TEST("int main() { double a = 5.5; char b = 'A'; return a & b; }"),
    TEST("int main() { int a = 2; double b = 3.5; return a % b; }"),
    TEST("int main() { double a = 3.5; char b = 'A'; return a + b * 2.0; }"),
    TEST("int main() { double a = 2.5; int b = 3; char c = 'A'; return a * (b - c); }")*/

};

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

int main(int argc, char * argv []) {
    initializeTerminal();
    std::cout << color::gray << "The one and only Tiny-C brought to you by NI-GEN" << color::reset << std::endl;
    // parse arguments
    char const * filename = nullptr;
    if (Options::parseArgs(argc, argv, filename) == EXIT_FAILURE)
        return EXIT_FAILURE;
    if (filename == nullptr) {
        size_t ntests = sizeof(tests) / sizeof(Test);
        size_t fails = 0;
        std::cout << "Running " << ntests << " tests..." << std::endl;
        for (auto const & t : tests) {
            if (! compile(t.input, & t)) {
                std::cout << color::red << t.file << ":" << t.line << ": Test failed." << color::reset << std::endl;
                std::cout << "    " << t.input << std::endl;
                ++fails;
                if (Options::exitAfterFailure)
                    break;
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
