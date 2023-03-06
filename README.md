# TinyC Project Repository

This project is a simple `cmake` affair that should work with any relatively modern (as of 2023) `C++` compiler on all three main platforms. If you are unfamiliar with cmake, to build & run the project, do the following (and learn `cmake`:):

    mkdir build
    cmake ..
    cmake --build .

> The first line creates the build directory where all the build artefacts and the executable will be put. The second line instructs cmake to generate build files for the project whose root is in the parent directory, which essentially creates the makefile, or MSVC project file, or something else depending on your build system. The last line then instructs cmake to build the project using the previously generated build scripts. 

To run, start `tinycc` from the build directory. You can pass it `--verbose` to enable debug prints, any other argument will be treated as a path to file to be compiled. If no file is specified, `tinycc` will execute all tests defined in the `main.cpp` file. 

## Resources

- [language reference](LANGUAGE.md)
- [courses](https://courses.fit.cvut.cz/NI-GEN/)

