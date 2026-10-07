#include <iostream>
#include "Repository.h"
using namespace std;

// show one commit on screen
void printCommit(Commit c) {
    cout << "Commit " << c.id << " | " << c.message << "\n";
    cout << "  Author: " << c.author << " | Parent: ";
    if (c.parent == 0) cout << "none";
    else cout << c.parent;
    cout << " | " << c.time << "\n";
}

int main() {
    Repository* repo = NULL;
    int choice;
    string name, text, author;

    cout << "Enter your name: ";
    getline(cin, author);

    while (true) {
        cout << "\n--- CodeGram (" << author << ") ---\n";
        if (repo != NULL)
            cout << "Repo: " << repo->getName() << " | Branch: " << repo->getCurrentBranch() << "\n";
        cout << "1. Create repository\n";
        cout << "2. Add file\n";
        cout << "3. Delete file\n";
        cout << "4. List files\n";
        cout << "5. Commit\n";
        cout << "6. History (current branch)\n";
        cout << "7. List all commits\n";
        cout << "8. View commit\n";
        cout << "9. Create branch\n";
        cout << "10. Switch branch\n";
        cout << "11. List branches\n";
        cout << "12. Delete branch\n";
        cout << "0. Exit\n";
        cout << "Choose: ";
        cin >> choice;
        cin.ignore();

        if (choice == 0)
            break;

        if (choice == 1) {
            cout << "Repository name: ";
            getline(cin, name);
            delete repo;
            repo = new Repository(name, author);
            cout << "Repository created.\n";
        }
        else if (repo == NULL) {
            cout << "Create a repository first.\n";
        }
        else if (choice == 2) {
            cout << "File name: ";
            getline(cin, name);
            cout << "File content: ";
            getline(cin, text);
            repo->addFile(name, text);
            cout << "File saved.\n";
        }
        else if (choice == 3) {
            cout << "File name: ";
            getline(cin, name);
            if (repo->deleteFile(name)) cout << "File deleted.\n";
            else cout << "File not found.\n";
        }
        else if (choice == 4) {
            map<string, string> files = repo->listFiles();
            if (files.empty()) cout << "No files.\n";
            for (auto f : files)
                cout << f.first << " : " << f.second << "\n";
        }
        else if (choice == 5) {
            cout << "Commit message: ";
            getline(cin, text);
            int id = repo->commit(text, author);
            if (id == 0) cout << "Nothing to commit.\n";
            else cout << "Commit " << id << " created.\n";
        }
        else if (choice == 6) {
            vector<Commit> history = repo->getHistory();
            if (history.empty()) cout << "No commits yet.\n";
            for (Commit c : history)
                printCommit(c);
        }
        else if (choice == 7) {
            vector<Commit> all = repo->listCommits();
            if (all.empty()) cout << "No commits yet.\n";
            for (Commit c : all)
                printCommit(c);
        }
        else if (choice == 8) {
            int id;
            cout << "Commit number: ";
            cin >> id;
            cin.ignore();
            Commit c(0, 0, "", "", map<string, string>());
            if (repo->getCommit(id, c)) {
                printCommit(c);
                for (auto f : c.files)
                    cout << "  - " << f.first << " : " << f.second << "\n";
            }
            else cout << "Commit not found.\n";
        }
        else if (choice == 9) {
            cout << "Branch name: ";
            getline(cin, name);
            if (repo->createBranch(name, author)) cout << "Branch created.\n";
            else cout << "Could not create branch.\n";
        }
        else if (choice == 10) {
            cout << "Branch name: ";
            getline(cin, name);
            if (repo->switchBranch(name)) cout << "Switched to " << name << ".\n";
            else cout << "Could not switch (not found or uncommitted changes).\n";
        }
        else if (choice == 11) {
            vector<Branch> list = repo->listBranches();
            for (Branch b : list) {
                if (b.name == repo->getCurrentBranch()) cout << "* ";
                else cout << "  ";
                cout << b.name << " | by " << b.author << " | latest commit: " << b.head << "\n";
            }
        }
        else if (choice == 12) {
            cout << "Branch name: ";
            getline(cin, name);
            if (repo->deleteBranch(name)) cout << "Branch deleted.\n";
            else cout << "Could not delete (not found, current, or main).\n";
        }
        else {
            cout << "Invalid choice.\n";
        }
    }

    delete repo;
    return 0;
}