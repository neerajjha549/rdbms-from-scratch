# ToyDB: A Simple B+ Tree Based Database from Scratch in C++

Welcome to ToyDB! This is an educational project to build a simple, persistent, single-table database from the ground up in C++. The goal is to demystify how databases work by implementing the core components, starting with the storage engine.

This project is inspired by and follows the excellent tutorial at [cstack.github.io/db_tutorial/](https://cstack.github.io/db_tutorial/ "null").

## 🌟 Current Features

* **Persistent Storage**: All data is saved to a binary file and reloaded on startup.
* **REPL Interface**: A simple Read-Eval-Print-Loop for interacting with the database.
* **Query Planner & Virtual Machine**: The database now compiles SQL into a custom bytecode format and executes it on a stack-based virtual machine.
* **SQL Parser & AST**: The database has a recursive descent parser that builds an Abstract Syntax Tree.
* **SQL Tokenizer**: Handles lexical analysis, breaking a raw SQL string into tokens.
* **Feature-Complete B+ Tree**: Data is stored and indexed in a B+ Tree, supporting efficient insertion, searching, and deletion with node splitting and merging.
* **Basic CRUD Operations**:
    
    -   `insert <id> <username> <email>`
        
    -   `select` (performs an efficient scan across the leaf nodes)
        
    -   `delete <id>` (removes a key and rebalances the tree if necessary)
        
* **Meta-Commands**:
    
    -   `.exit`: To exit the application and save the database file.
        
    -   `.constants`: To print internal layout constants.
        
    -   `.btree`: To print a visualization of the B-Tree structure.

## 🛠️ Building the Database

### Prerequisites

To build and run this project, you will need a C++ compiler (`g++`) and the `make` utility.

-   **On Debian/Ubuntu Linux**:
    
    ```
    sudo apt update
    sudo apt install build-essential
    
    ```
    
-   **On Windows**: You can use MinGW-w64 through MSYS2 or another distribution. Ensure that `g++` and `make` are in your system's PATH.
    

### Compilation

Once you have the prerequisites, simply run `make` in the project's `src` directory:

```
make

```

This will compile all the source files and create an executable named `db` (or `db.exe` on Windows).

To clean up the build files, you can run in the project's `src` directory:

```
make clean

```

This command removes the executable and all intermediate object files.

## 🚀 Running and Usage

To start the database, provide a filename as an argument. If the file doesn't exist, it will be created.

```
./db mydatabase.db

```

### Supported Commands

**Insert a row:**

```
db > insert 1 user1 user1@example.com
Executed.

```

**Select all rows:**

```
db > select
(1, user1, user1@example.com)
Executed.

```

**Delete a row:**

```
db > delete 1
Executed.

```
## 🏗️ Architecture Overview

The query lifecycle is now: `Input String` -> `Tokenizer` -> `Parser (AST)` -> `Query Planner (Bytecode)` -> `Virtual Machine (Execution)`.

* **`main.cpp`**: Orchestrates the tokenizing, parsing, planning, and execution pipeline.
* **`vm.cpp` / `vm.h`**: A stack-based virtual machine that executes bytecode.
* **`query_planner.cpp` / `query_planner.h`**: Compiles the AST into a bytecode program for the VM.
* **`parser.cpp` / `parser.h`**: Builds an Abstract Syntax Tree (AST) from a stream of tokens.
* **`ast.h`**: Defines the data structures for the AST.
* **`tokenizer.cpp` / `tokenizer.h`**: Handles lexical analysis.
* **`pager.cpp` / `pager.h`**: Manages reading and writing pages of data.
* **`table.cpp` / `table.h`**: Provides the high-level API for data operations.
* **`btree.cpp` / `btree.h`**: The B+ Tree storage engine implementation.
* **`row.h`**: Defines the `Row` structure and serialization.
* **`common.h`**: Contains common includes and project-wide constants.

## 🗺️ Project Roadmap

We are building the final layers of the database.
1.  **Transaction Management**: Add ACID compliance through a Write-Ahead Log (WAL) and concurrency control.
2.  **Data Types & Catalogs**: Move beyond a single, hard-coded table structure to support multiple tables, schemas, and data types.
