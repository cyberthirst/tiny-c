# TinyVerse Repository Collection

> This is the 2023 revision and hopefully a standardization too. 

This project is a simple `cmake` affair that should work with any relatively modern (as of 2023) `C++` compiler on all three main platforms. If you are unfamiliar with cmake, to build & run the project, do the following (and learn `cmake`:):

    mkdir build
    cmake ..
    cmake --build .

> The first line creates the build directory where all the build artefacts and the executable will be put. The second line instructs cmake to generate build files for the project whose root is in the parent directory, which essentially creates the makefile, or MSVC project file, or something else depending on your build system. The last line then instructs cmake to build the project using the previously generated build scripts. 

To run, start `tinyc` from the build directory. You can pass it `--verbose` to enable debug prints, any other argument will be treated as a path to file to be compiled. If no file is specified, `tinyc` will execute all tests defined in the `main.cpp` file. 

## `tinyC` Language Frontend

*tinyC* is a *C*-like language with much smaller subset of types (`int`, `char`, `double`, arrays and structs, function pointers and pointers) and limited set of features. TinyC's semantics and grammar are simpler, which makes tinyC not a strict subset of C.

## Language Reference

    PROGRAM := { FUN_DECL | VAR_DECLS ';' | STRUCT_DECL | FUNPTR_DECL }

### Functions

    FUN_DECL := TYPE_FUN_RET identifier '(' [ FUN_ARG { ',' FUN_ARG } ] ')' [ BLOCK_STMT ]
    FUN_ARG := TYPE identifier

Functions are declared the same way as in C. A function definition starts with the return type, followed by function name and arguments. Arguments have type and name. Default values or variadic arguments are not supported. Functions can be declared or defined. Function definitions must have their body.

### Statements

    STATEMENT := BLOCK_STMT | IF_STMT | SWITCH_STMT | WHILE_STMT | DO_WHILE_STMT | FOR_STMT | BREAK_STMT | CONTINUE_STMT | RETURN_STMT | EXPR_STMT

    BLOCK_STMT := '{' { STATEMENT } '}'

    IF_STMT := if '(' EXPR ')' STATEMENT [ else STATEMENT ]

    SWITCH_STMT := switch '(' EXPR ')' '{' { CASE_STMT } [ default ':' CASE_BODY } ] { CASE_STMT } '}'
    CASE_STMT := case integer_literal ':' CASE_BODY
    CASE_BODY := { STATEMENT }

    WHILE_STMT := while '(' EXPR ')' STATEMENT
    DO_WHILE_STMT := do STATEMENT while '(' EXPR ')' ';'
    FOR_STMT := for '(' [ EXPR_OR_VAR_DECL ] ';' [ EXPR ] ';' [ EXPR ] ')' STATEMENT

    BREAK_STMT := break ';'
    CONTINUE_STMT := continue ';'

    RETURN_STMT := return [ EXPR ] ';'

    EXPR_STMT := EXPR_OR_VAR_DECL ';'

### Types

To keep the type declarations similar to those of `C` while keeping the grammar simple to parse, the type declarations grammar is a bit repetitive:

    TYPE := (int | double | char | identifier) { * }
         |= void * { * }

    TYPE_FUN_RET := TYPE | void

Pointers can point to either a plain type, an identifier representing a struct or function pointer type, or `void`. Pointers to pointers are allowed.

### Type Declarations

    STRUCT_DECL := struct identifier [ '{' { TYPE identifier ';' } '}' ] ';'

Structured types must always be declared before they are used. Forward declarations are supported as well.

    FUNPTR_DECL := typedef TYPE_FUN_RET '(' '*' identifier ')' '(' [ TYPE { ',' TYPE } ] ')' ';'

Function pointers must always be declared before they can be used.


### Expressions

    F := integer | double | char | string | identifier | '(' EXPR ')' | E_CAST
    E_CAST := cast '<' TYPE '>' '(' EXPR ')'

    E_CALL_INDEX_MEMBER_POST := F { E_CALL | E_INDEX | E_MEMBER | E_POST }
    E_CALL := '(' [ EXPR { ',' EXPR } ] ')'
    E_INDEX := '[' EXPR ']'
    E_MEMBER := ('.' | '->') identifier
    E_POST := '++' | '--'

    E_UNARY_PRE := { '+' | '-' | '!' | '~' | '++' | '--' | '*' | '&' } E_CALL_INDEX_MEMBER_POST

Unary prefix operators are either simple arithmetics (plus, minus, not and neg), the increment and decrement operators (these are a bit harder to compile as they immediately update the value of their argument) and the special operators for dereferencing ('*') and getting an address of variables ('&').

    E1 := E_UNARY_PRE { ('*' | '/' | '%' ) E_UNARY_PRE }
    E2 := E1 { ('+' | '-') E1 }
    E3 := E2 { ('<<' | '>>') E2 }
    E4 := E3 { ('<' | '<=' | '>' | '>=') E3 }
    E5 := E4 { ('==' | '!=') E4 }
    E6 := E5 { '&' E5 }
    E7 := E6 { '|' E6 }
    E8 := E7 { '&&' E7 }
    E9 := E8 { '||' E8 }

Basic arithmetic operators are supported in the same way they are in `c/c++` including their priority. All evaluate left to right (left associative).

    EXPR := E9 [ '=' EXPR ]
    EXPRS := EXPR { ',' EXPR }

The lowest priority is the assignment operator. Note that assignment operator is right-to-left (right associative).

Expressions can be separated by a comma, they will be evaluated left to right.

    VAR_DECL := TYPE identifier [ '[' E9 ']' ] [ '=' EXPR ]
    VAR_DECLS := VAR_DECL { ',' VAR_DECL }

    EXPR_OR_VAR_DECL := VAR_DECLS | EXPRS

Variable declaration must start with a type specification. Multiple variables of same type cannot be declared in a single expression, but multiple comma separated declarations with explicit type are allowed.

Optionally, arrays of statically known size may be defined with `[]` operator and the index after the variable name.


