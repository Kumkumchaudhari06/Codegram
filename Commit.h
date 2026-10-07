#ifndef COMMIT_H
#define COMMIT_H

#include <string>
#include <map>
using namespace std;

class Commit {
public:
    int id;                     // commit number
    int parent;                 // previous commit id (0 = none)
    string author;              // who made the commit
    string message;
    string time;                // when it was made
    map<string, string> files;  // saved copy of files

    Commit(int id, int parent, string author, string message, map<string, string> files);
};

#endif