#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <string>
#include <map>
#include <vector>
#include "Commit.h"
#include "Branch.h"
using namespace std;

// Repository only does the work and returns data.
// Printing is done in main.cpp, so a future app can reuse this class.
class Repository {
private:
    string name;
    string owner;
    string currentBranch;
    map<string, string> files;       // current files
    vector<Commit> commits;          // all commits
    map<string, Branch> branches;    // all branches

    bool hasChanges();

public:
    Repository(string name, string owner);

    string getName();
    string getOwner();
    string getCurrentBranch();

    // files
    void addFile(string filename, string content);
    bool deleteFile(string filename);
    map<string, string> listFiles();

    // commits
    int commit(string message, string author);      // returns id, 0 if failed
    vector<Commit> getHistory();                    // commits of current branch
    vector<Commit> listCommits();                   // all commits
    bool getCommit(int id, Commit &result);

    // branches
    bool createBranch(string branchName, string author);
    bool deleteBranch(string branchName);
    bool switchBranch(string branchName);
    vector<Branch> listBranches();
};

#endif