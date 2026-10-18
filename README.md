# Codegram
### Intelligent Version Control and Conflict Resolution

Codegram is a C++-based version control project designed to help developers manage code changes, organize branches, track commits, and understand merge conflicts. It aims to make version control easier to learn and use through structured data handling and efficient change management.

## Features

- **Repository Management** – Create and manage a local repository.
- **Commit Management** – Store changes with commit messages and history.
- **Branch Management** – Organize work into separate branches.
- **File Tracking** – Track changes made to project files.
- **Staging** – Prepare selected files before committing.
- **Diff Engine** – Compare file versions and identify changes.
- **Merge Conflict Resolution** – Support the identification and resolution of conflicting changes.
- **Dependency Graph** – Represent relationships between project components.
- **Compression Utilities** – Provide utilities for handling compressed data.

## Technologies Used

- **Language:** C++
- **Standard:** C++17
- **Data Structures:** Linked Lists, Trees, Graphs, Stacks, Queues, and Dynamic Arrays
- **Core Concepts:** Object-Oriented Programming, File Handling, Hashing, and Algorithms


## How It Works

1. Initialize a local repository.
2. Track files and identify changes.
3. Stage the files that need to be saved.
4. Create commits to maintain a record of changes.
5. Manage branches for separate development work.
6. Compare versions and handle merge conflicts.
7. Use dependency relationships to support root-cause analysis.

## Getting Started

### Prerequisites

- A C++ compiler supporting C++17 or later.
- Visual Studio Code or another C++ development environment.
- A terminal for compiling and running the project.

### Compile

Compile the project's source files together with the main entry-point file. For example, using GCC:

```bash
g++ -std=c++17 main.cpp repomanager.cpp branchmanager.cpp commitmanager.cpp dependencygraph.cpp compression.cpp -o codegram
```

**Important:** This example assumes the listed source files can be compiled independently with your current implementation. If other `.cpp` files are required, or if the project uses additional dependencies, include them in the build command.

### Run

On Windows PowerShell:

```powershell
.\codegram.exe
```

On Linux:

```bash
./codegram
```

## Project Objective

The objective of Codegram is to build a beginner-friendly version control system using C++ and data structures. The project focuses on understanding how repositories, commits, branches, file changes, and merge conflicts can be managed through core programming concepts.

## Future Improvements

- Complete integration between the backend and graphical interface.
- Improve automated merge handling.
- Enhance root-cause analysis using commit history and dependency relationships.
- Add comprehensive testing and validation.
- Improve usability and error reporting.

## Team Code Blooded

**Project:** Codegram – Intelligent Version Control and Conflict Resolution

**Project ID:** DSCPP-III-2026-T156

Developed as a project focused on C++, data structures, algorithms, and practical version control concepts.

---

*Codegram — Track changes. Manage branches. Resolve conflicts.*
