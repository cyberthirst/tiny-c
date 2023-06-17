## Terminator instructions
These classes are subclasses of the Instruction class, which represents an abstract representation of instructions in an intermediate language. They each represent a different type of instruction related to branching, terminating, and control flow in a program.

Instruction::TerminatorB: This class represents a basic terminator instruction with a single target basic block. It is a subclass of Instruction::Terminator. The target member variable holds a pointer to the target BasicBlock. These instructions are used to control the flow of the program by jumping to a specific basic block.

Instruction::TerminatorReg: This class represents a terminator instruction that takes a single register as input. It is a subclass of Instruction::Terminator. The reg member variable holds a pointer to the Instruction that represents the register. These instructions are used to control the flow of the program based on the value of the specified register.

Instruction::TerminatorRegBB: This class represents a terminator instruction that takes a single register and two basic blocks as input. It is a subclass of Instruction::Terminator. The reg member variable holds a pointer to the Instruction that represents the register, while target1 and target2 hold pointers to the target BasicBlocks. These instructions are used to control the flow of the program based on the value of the specified register, and they branch to one of the two target basic blocks depending on the value.

These classes have different constructors to initialize the member variables with default values or specified values. They also override the print method to provide custom printing behavior for each subclass. The Instruction class provides several common member variables, such as opcode, type, ast, and name, and methods to generate unique names for each instruction.

## \#define INS(NAME, ENCODING) macro
This macro is a shorthand for defining functions that create new instances of various instruction classes. The macro INS is defined to generate a function with a specific name and return type. The function takes a variable number of arguments and returns a pointer to a newly created instruction object. After defining the macro, the "insns.h" file is included to actually create the functions using the INS macro.

Let's break down the macro definition:

```cpp
#define INS(NAME, ENCODING) \
template<typename... Args> \
Instruction::ENCODING * NAME(Args... args) { return new Instruction::ENCODING{Opcode::NAME, args...}; }
```
`#define INS(NAME, ENCODING)` declares the macro INS with two parameters: NAME and ENCODING.
`template<typename... Args>` is a variadic template that allows the function to accept a variable number of arguments of any type.
`Instruction::ENCODING * NAME(Args... args)` declares the function with the name NAME that takes a variable number of arguments and returns a pointer to an object of type `Instruction::ENCODING`.
`{ return new Instruction::ENCODING{Opcode::NAME, args...}; }` is the function body, which creates a new instance of `Instruction::ENCODING`, passing an opcode and the input arguments to the constructor, and returns a pointer to this new instance.
The "insns.h" file contains a list of INS macros, each defining a function for a specific instruction. When the "insns.h" file is included, the INS macro is used to create the functions with the specified names and types.

For example, let's take one of the instructions from "insns.h":

```cpp
INS(LDI, ImmI)
```

When the macro is expanded, the function will look like this:

```cpp
template<typename... Args>
Instruction::ImmI * LDI(Args... args) { return new Instruction::ImmI{Opcode::LDI, args...}; }
```
This function takes a variable number of arguments, creates a new instance of `Instruction::ImmI`, passes `Opcode::LDI` and the input arguments to the constructor, and returns a pointer to this new instance.

The same process is repeated for each instruction defined in "insns.h", generating functions for each of them.


LD: 600001f08000

