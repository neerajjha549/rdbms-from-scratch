# ToyDB: A Simple B+ Tree Based Database from Scratch in C++

Welcome to ToyDB! This is an educational project to build a simple, persistent, single-table database from the ground up in C++. The goal is to demystify how databases work by implementing the core components, starting with the storage engine.

This project is inspired by and follows the excellent tutorial at [cstack.github.io/db_tutorial/](https://cstack.github.io/db_tutorial/ "null").

## 🌟 Current Features

-   **Persistent Storage**: All data is saved to a binary file and reloaded on startup.
    
-   **REPL Interface**: A simple Read-Eval-Print-Loop for interacting with the database.
    
-   **Feature-Complete B+ Tree for Indexing**: Data is stored and indexed in a robust B+ Tree structure.
    
    -   All leaf nodes are linked sequentially, allowing for highly efficient full-table scans.
        
    -   Supports splitting leaf and internal nodes recursively up to the root.
        
    -   Supports deletion with node merging and rebalancing to maintain tree structure and performance.
        
-   **Basic CRUD Operations**:
    
    -   `insert <id> <username> <email>`
        
    -   `select` (performs an efficient scan across the leaf nodes)
        
    -   `delete <id>` (removes a key and rebalances the tree if necessary)
        
-   **Meta-Commands**:
    
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

-   **`main.cpp`**: Contains the REPL and handles parsing user input.
    
-   **`pager.cpp` / `pager.h`**: Manages reading and writing pages of data from the database file to memory.
    
-   **`table.cpp` / `table.h`**: Provides a high-level API for interacting with the data (`Table` and `Cursor`).
    
-   **`btree.cpp` / `btree.h`**: The heart of the storage engine. Contains the logic for the B+ Tree data structure.
    
-   **`row.cpp` / `row.h`**: Defines the `Row` structure and its serialization/deserialization logic.
    

## 🗺️ Project Roadmap

The B+ Tree storage engine is now functionally complete. The next major phases involve building the layers on top of it to turn it into a true RDBMS.

1.  **SQL Compiler**: Implement a proper SQL front-end with a parser (using Flex/Bison) and a query planner.
    
2.  **Transaction Management**: Add ACID compliance through a Write-Ahead Log (WAL) for durability and concurrency control mechanisms like MVCC.
    
3.  **Data Types & Catalogs**: Move beyond a single, hard-coded table structure to support multiple tables, different data types, and a system catalog (`information_schema`).