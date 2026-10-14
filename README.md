# Codegram
### Intelligent Version Control and Conflict Resolution

Codegram is a C++-based version control system designed to simplify repository management, track file changes, manage commits and branches, and make merge conflict resolution easier through a user-friendly graphical interface.

The project combines Object-Oriented Programming, Data Structures and Algorithms, file handling, and local storage to demonstrate how version control works internally.

## Features

- **Repository Management:** Create and manage local repositories.
- **File Tracking:** Identify modified, deleted, tracked, and untracked files.
- **Staging:** Select changes before committing them.
- **Commit Management:** Save project snapshots with commit messages and identifiers.
- **Branch Management:** Maintain separate development branches.
- **Commit History:** Track previous commits and project changes.
- **Diff Viewer:** Compare file versions and identify modifications.
- **Merge Engine:** Combine changes from different branches.
- **Conflict Resolution:** Provide options such as Keep Current, Keep Incoming, and Keep Both.
- **Root-Cause Finder:** Analyze commit history and file dependencies to help identify possible sources of issues.
- **Graphical Interface:** Provide a desktop interface using Qt Widgets.

## Tech Stack

- **Language:** C++
- **Standard:** C++17
- **GUI Framework:** Qt Widgets
- **Programming Concepts:** OOP, file handling, hashing, and modular design
- **Data Structures:** Linked lists, stacks, queues, trees, graphs, dynamic arrays, sets, and priority queues
- **Storage:** Local file system
- **Development Environment:** Visual Studio Code, GCC (MSYS2 UCRT64), and CMake
- **Version Control:** Git and GitHub

## System Architecture

Codegram follows a layered architecture:

Qt Widgets GUI
       ↓
Controller Layer
       ↓
C++ Backend Modules
       ↓
Local Repository Storage

The GUI handles user interaction, the controller connects interface actions to backend operations, and the backend processes version control tasks using C++ and data structures. Repository data is stored locally without requiring an external database.

## Project Structure

```text
Codegram/
├── CMakeLists.txt
├── main.cpp
├── src/
│   ├── utils/
│   │   ├── FileIO.h
│   │   ├── FileIO.cpp
│   │   ├── Sha256.h
│   │   └── Sha256.cpp
│   ├── storage/
│   │   └── Serializer.cpp
│   └── modules/
│       ├── Repository
│       ├── Commits
│       ├── Branches
│       ├── History
│       ├── Merge
│       └── RootCauseAnalyzer
└── README.md
```

*Note: The structure above is illustrative. Update the directory tree to match the actual files in the repository.*

## Development Status

Codegram is being developed in phases.

- **Backend Development:** Core repository, commit, branch, and supporting storage utilities are being developed.
- **Data Structures:** Core version-control operations use appropriate data structures and algorithms.
- **GUI Development:** Qt Widgets interface and controller integration are in progress.
- **Integration and Testing:** End-to-end workflows, merge handling, and validation remain part of ongoing development.

Some advanced features are still under development and may not yet be fully integrated.

## Getting Started

### Prerequisites

- A C++17-compatible compiler
- CMake
- Visual Studio Code (recommended)
- Qt 6 development libraries for the graphical interface

### Build and Run

After configuring the project and installing the required dependencies, build it using CMake:

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

Run the generated executable from the build directory. The exact executable path depends on the CMake configuration.

**Note:** Ninja must be installed and available in PATH to use the commands above. The Qt GUI build also requires the appropriate Qt configuration.

## Future Improvements

- Complete GUI and controller integration.
- Improve visual file comparison and conflict resolution.
- Enhance automatic merge handling.
- Refine root-cause analysis using dependency relationships and commit history.
- Add comprehensive testing and error handling.
- Improve repository integrity and performance.

## Team

**Team Name:** Code Blooded  
**Team ID:** DSCPP-III-2026-T156

- **Kumkum Chaudhari** — Team Lead; Backend and GUI Development
- **Rakshita** — Backend Development; Repository, Commits, and Branches
- **Mansi Gusain** — Root-Cause Analysis and GUI Development
- **Arpit Panwar** — File Tracking, Staging, and History

## Project Objective

The objective of Codegram is to make version control easier to understand and use while demonstrating the practical application of C++, OOP, data structures, and algorithms in a modular software engineering project.

## License

This project is developed for academic and educational purposes. A formal open-source license can be added when the project is ready for public distribution.


