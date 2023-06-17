//
// Created by Mirek Škrabal on 17.06.2023.
//

#include "struct_tests.h"

std::vector<Test> struct_tests{
    TEST("struct Point { int x; int y; }; int main() { struct Point p; p.x = 5; p.y = 6; return p.x + p.y; }"),
    TEST("struct Point { int x; int y; }; struct Line { struct Point p1; struct Point p2; }; int main() { struct Line l; l.p1.x = 1; l.p1.y = 2; l.p2.x = 3; l.p2.y = 4; return l.p1.x + l.p1.y + l.p2.x + l.p2.y; }"),
    TEST("struct Point { int x; int y; }; struct Box { struct Point topLeft; struct Point bottomRight; }; int main() { struct Box b; b.topLeft.x = 1; b.topLeft.y = 2; b.bottomRight.x = 5; b.bottomRight.y = 6; return b.topLeft.x + b.topLeft.y + b.bottomRight.x + b.bottomRight.y; }"),
    TEST("struct Node; struct Node { int value; struct Node *next; }; int main() { struct Node n1, n2; n1.value = 5; n1.next = &n2; n2.value = 6; n2.next = 0; return n1.value + n1.next->value; }"),
    TEST("struct Foo { }; void main(Foo x) {}"),
    TEST("struct Foo; struct Foo { int i; }; void main(Foo x) {}"),
};

DEFINE_TEST_CATEGORY(struct_tests)
