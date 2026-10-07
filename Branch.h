#ifndef BRANCH_H
#define BRANCH_H

#include <string>
using namespace std;

class Branch {
public:
    string name;
    string author;   // who created the branch
    int head;        // latest commit id on this branch (0 = none)

    Branch();
    Branch(string name, string author, int head);
};

#endif