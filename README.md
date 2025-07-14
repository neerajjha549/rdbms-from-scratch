# ToyDB - rdbms-from-scratch
Welcome to ToyDB! This is an educational project to build a simple, persistent, single-table database from the ground up in C++. The goal is to demystify how databases work by implementing the core components, starting with the storage engine.

This project is inspired by and follows the excellent tutorial at [cstack.github.io/db_tutorial/](https://cstack.github.io/db_tutorial/ "null").

## 🌟 Current Features

-   **Persistent Storage**: All data is saved to a binary file and reloaded on startup.
    
-   **REPL Interface**: A simple Read-Eval-Print-Loop for interacting with the database.
    
-   **B-Tree for Indexing**: Data is stored and indexed in a B-Tree structure, allowing for efficient lookups. The tree currently supports:
    
    -   Splitting the root node.
        
    -   Splitting leaf nodes and updating the parent internal node.
        
    -   Splitting internal nodes recursively up to the root.
        
-   **Basic CRUD Operations**:
    
    -   `insert <id> <username> <email>`
        
    -   `select` (scans the entire table)
        
    -   `delete <id>` (removes a key; does not yet merge nodes)
        
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

Once you have the prerequisites, simply run `make` in the project's root directory:

```
make

```

This will compile all the source files and create an executable named `db` (or `db.exe` on Windows).

To clean up the build files, you can run:

```
make clean

```

This command removes the executable and all intermediate object files.

## 🚀 Running and Usage

To start the database, provide a filename as an argument. If the file doesn't exist, it will be created.

```
./db mydatabase.db

```

This will open the REPL interface:

```
db >

```

### Supported Commands

You can now issue SQL-like commands or meta-commands.

**Insert a row:**

```
db > insert 1 user1 user1@example.com
Executed.
db > insert 2 user2 user2@example.com
Executed.

```

**Select all rows:**

```
db > select
(1, user1, user1@example.com)
(2, user2, user2@example.com)
Executed.

```

**Delete a row:**

```
db > delete 1
Executed.
db > select
(2, user2, user2@example.com)
Executed.

```

**Exit the database:**

```
db > .exit
Bye!

```

## 🏗️ Architecture Overview

The database is designed with a modular architecture, separating different concerns into their own files.

-   **`main.cpp`**: Contains the REPL and handles parsing user input.
    
-   **`pager.cpp` / `pager.h`**: The Pager is responsible for reading and writing pages of data (fixed at 4KB) from the database file to memory. It maintains an in-memory cache of pages.
    
-   **`table.cpp` / `table.h`**: Defines the `Table` and `Cursor` structures, which provide a high-level API for interacting with the data.
    
-   **`btree.cpp` / `btree.h`**: The heart of the storage engine. This contains all the logic for the B-Tree data structure, including node layout definitions, searching, insertion, splitting, and deleting from nodes.
    
-   **`row.cpp` / `row.h`**: Defines the `Row` structure and contains the serialization/deserialization logic to convert a `Row` to and from its compact binary representation for storage.
    

## 🗺️ Project Roadmap

This project is a work in progress. The next major milestones are:

1.  **B+ Tree Conversion**: Evolve the B-Tree into a B+ Tree by adding sibling pointers to all leaf nodes. This will make full table scans (`select`) much more efficient, as they can traverse a linked list of leaves instead of going up and down the tree.
    
2.  **Deletion with Rebalancing**: Implement node merging and rebalancing when a deletion causes a node to become under-utilized.
    
3.  **SQL Compiler**: Implement a proper SQL front-end with a parser (using Flex/Bison) and a query planner.
    
4.  **Transaction Management**: Add ACID compliance through a Write-Ahead Log (WAL) for durability and concurrency control mechanisms like MVCC.