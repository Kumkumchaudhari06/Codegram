#include "Commit.h"
#include <ctime>

Commit::Commit(int id, int parent, string author, string message, map<string, string> files) {
    this->id = id;
    this->parent = parent;
    this->author = author;
    this->message = message;
    this->files = files;

    time_t now = ::time(0);
    string t = ctime(&now);
    t.pop_back();               // remove the newline at the end
    this->time = t;
}