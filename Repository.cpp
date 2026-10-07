#include "Repository.h"

Repository::Repository(string name, string owner) {
    this->name = name;
    this->owner = owner;
    currentBranch = "main";
    branches["main"] = Branch("main", owner, 0);    // first branch
}

string Repository::getName() { return name; }
string Repository::getOwner() { return owner; }
string Repository::getCurrentBranch() { return currentBranch; }

// ---------- files ----------

void Repository::addFile(string filename, string content) {
    files[filename] = content;
}

bool Repository::deleteFile(string filename) {
    return files.erase(filename) > 0;
}

map<string, string> Repository::listFiles() {
    return files;
}

// ---------- commits ----------

bool Repository::hasChanges() {
    int head = branches[currentBranch].head;
    if (head == 0)
        return !files.empty();
    return commits[head - 1].files != files;
}

int Repository::commit(string message, string author) {
    if (!hasChanges())
        return 0;
    int parent = branches[currentBranch].head;
    int id = commits.size() + 1;
    commits.push_back(Commit(id, parent, author, message, files));
    branches[currentBranch].head = id;      // branch now points to new commit
    return id;
}

vector<Commit> Repository::getHistory() {
    vector<Commit> history;
    int id = branches[currentBranch].head;
    while (id != 0) {                       // follow parents backwards
        history.push_back(commits[id - 1]);
        id = commits[id - 1].parent;
    }
    return history;
}

vector<Commit> Repository::listCommits() {
    return commits;
}

bool Repository::getCommit(int id, Commit &result) {
    if (id < 1 || id > (int)commits.size())
        return false;
    result = commits[id - 1];
    return true;
}

// ---------- branches ----------

bool Repository::createBranch(string branchName, string author) {
    if (branchName == "" || branches.count(branchName))
        return false;
    branches[branchName] = Branch(branchName, author, branches[currentBranch].head);
    return true;
}

bool Repository::deleteBranch(string branchName) {
    if (!branches.count(branchName)) return false;
    if (branchName == currentBranch) return false;  // cannot delete current branch
    if (branchName == "main") return false;         // protect main
    branches.erase(branchName);
    return true;
}

bool Repository::switchBranch(string branchName) {
    if (!branches.count(branchName)) return false;
    if (hasChanges()) return false;                 // commit first
    currentBranch = branchName;
    int head = branches[branchName].head;
    files.clear();
    if (head != 0)
        files = commits[head - 1].files;            // load files of that branch
    return true;
}

vector<Branch> Repository::listBranches() {
    vector<Branch> list;
    for (auto b : branches)
        list.push_back(b.second);
    return list;
}