#include "Branch.h"

Branch::Branch() {
    name = "";
    author = "";
    head = 0;
}

Branch::Branch(string name, string author, int head) {
    this->name = name;
    this->author = author;
    this->head = head;
}