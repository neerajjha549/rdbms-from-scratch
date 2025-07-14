#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h> // For file operations - compatibility with POSIX systems

// Define fixed-size constants for our table schema.
#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

// Row structure representing a single record in our 'users' table.
struct Row {
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE + 1]; // +1 for null terminator
    char email[COLUMN_EMAIL_SIZE + 1];   // +1 for null terminator
};

// --- Serialization & Deserialization ---
const uint32_t ID_SIZE = sizeof(uint32_t);
const uint32_t USERNAME_SIZE = sizeof(char) * (COLUMN_USERNAME_SIZE + 1);
const uint32_t EMAIL_SIZE = sizeof(char) * (COLUMN_EMAIL_SIZE + 1);
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;

void serialize_row(const Row* source, void* destination) {
    memcpy((char*)destination + ID_OFFSET, &(source->id), ID_SIZE);
    strncpy((char*)destination + USERNAME_OFFSET, source->username, USERNAME_SIZE);
    strncpy((char*)destination + EMAIL_OFFSET, source->email, EMAIL_SIZE);
}

void deserialize_row(const void* source, Row* destination) {
    memcpy(&(destination->id), (char*)source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), (char*)source + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(&(destination->email), (char*)source + EMAIL_OFFSET, EMAIL_SIZE);
}

/*
The Pager
What it is: The pager is responsible for reading and writing data to the database file in fixed-size chunks called pages (in our case, 4KB).

Why we need it: Disk access is extremely slow compared to memory access. The pager's primary job is to minimize disk I/O by keeping recently used pages in an in-memory cache. When a part of the database is needed, the pager checks if the corresponding page is already in the cache. If it is, it's a "cache hit," and the data is returned instantly. If not (a "cache miss"), the pager reads the page from the disk into the cache before returning it.

What it enables: This layer is the foundation upon which we will build our B-Tree index. The B-Tree will operate on pages, not raw bytes, and it will rely entirely on the pager to fetch and write the nodes of the tree.
*/

// --- Pager Abstraction ---
// Manages reading and writing pages to/from the database file.
// Implements a simple cache to reduce disk I/O.

const uint32_t PAGE_SIZE = 4096;
const uint32_t TABLE_MAX_PAGES = 100;

struct Pager {
    int file_descriptor;
    uint32_t file_length;
    void* pages[TABLE_MAX_PAGES];
};

Pager* pager_open(const std::string& filename) {
    int fd = open(filename.c_str(), O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
    if (fd == -1) {
        std::cerr << "Unable to open file" << std::endl;
        exit(EXIT_FAILURE);
    }

    off_t file_length = lseek(fd, 0, SEEK_END);

    Pager* pager = new Pager();
    pager->file_descriptor = fd;
    pager->file_length = file_length;

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        pager->pages[i] = nullptr;
    }

    return pager;
}

void* get_page(Pager* pager, uint32_t page_num) {
    if (page_num >= TABLE_MAX_PAGES) {
        std::cerr << "Tried to fetch page number out of bounds. " << page_num << " > " << TABLE_MAX_PAGES << std::endl;
        exit(EXIT_FAILURE);
    }

    // Cache hit. Return the cached page.
    if (pager->pages[page_num] != nullptr) {
        return pager->pages[page_num];
    }

    // Cache miss. Allocate memory and load from file.
    void* page = malloc(PAGE_SIZE);
    uint32_t num_pages = pager->file_length / PAGE_SIZE;

    // We might save a partial page at the end of the file
    if (pager->file_length % PAGE_SIZE) {
        num_pages += 1;
    }

    if (page_num < num_pages) {
        lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
        ssize_t bytes_read = read(pager->file_descriptor, page, PAGE_SIZE);
        if (bytes_read == -1) {
            std::cerr << "Error reading file: " << errno << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    pager->pages[page_num] = page;
    return page;
}

void pager_flush(Pager* pager, uint32_t page_num, uint32_t size) {
    if (pager->pages[page_num] == nullptr) {
        std::cerr << "Tried to flush null page." << std::endl;
        exit(EXIT_FAILURE);
    }

    lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
    ssize_t bytes_written = write(pager->file_descriptor, pager->pages[page_num], size);

    if (bytes_written == -1) {
        std::cerr << "Error writing to file: " << errno << std::endl;
        exit(EXIT_FAILURE);
    }
}

// --- Table and Cursor Abstractions ---
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

struct Table {
    Pager* pager;
    uint32_t num_rows;
};

// A cursor points to a location in the table.
struct Cursor {
    Table* table;
    uint32_t row_num;
    bool end_of_table; // Indicates a position one past the last element
};

Table* db_open(const std::string& filename) {
    Pager* pager = pager_open(filename);
    Table* table = new Table();
    table->pager = pager;
    table->num_rows = pager->file_length / ROW_SIZE;
    return table;
}

void db_close(Table* table) {
    Pager* pager = table->pager;
    uint32_t num_full_pages = table->num_rows / ROWS_PER_PAGE;

    for (uint32_t i = 0; i < num_full_pages; i++) {
        if (pager->pages[i] != nullptr) {
            pager_flush(pager, i, PAGE_SIZE);
            free(pager->pages[i]);
            pager->pages[i] = nullptr;
        }
    }

    // There may be a partial page to write to the end of the file
    uint32_t num_additional_rows = table->num_rows % ROWS_PER_PAGE;
    if (num_additional_rows > 0) {
        uint32_t page_num = num_full_pages;
        if (pager->pages[page_num] != nullptr) {
            pager_flush(pager, page_num, num_additional_rows * ROW_SIZE);
            free(pager->pages[page_num]);
            pager->pages[page_num] = nullptr;
        }
    }

    int result = close(pager->file_descriptor);
    if (result == -1) {
        std::cerr << "Error closing db file." << std::endl;
        exit(EXIT_FAILURE);
    }
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        void* page = pager->pages[i];
        if (page) {
            free(page);
            pager->pages[i] = nullptr;
        }
    }
    delete pager;
    delete table;
}

// Returns a pointer to the position of the row specified by the cursor.
void* cursor_value(Cursor* cursor) {
    uint32_t row_num = cursor->row_num;
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    void* page = get_page(cursor->table->pager, page_num);
    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    uint32_t byte_offset = row_offset * ROW_SIZE;
    return (char*)page + byte_offset;
}

void cursor_advance(Cursor* cursor) {
    cursor->row_num += 1;
    if (cursor->row_num >= cursor->table->num_rows) {
        cursor->end_of_table = true;
    }
}

// Create a cursor that points to the beginning of the table
Cursor* table_start(Table* table) {
    Cursor* cursor = new Cursor();
    cursor->table = table;
    cursor->row_num = 0;
    cursor->end_of_table = (table->num_rows == 0);
    return cursor;
}

// Create a cursor that points to the end of the table
Cursor* table_end(Table* table) {
    Cursor* cursor = new Cursor();
    cursor->table = table;
    cursor->row_num = table->num_rows;
    cursor->end_of_table = true;
    return cursor;
}

void print_row(const Row& row) {
    std::cout << "(" << row.id << ", " << row.username << ", " << row.email << ")" << std::endl;
}

// --- Statement & REPL ---
enum StatementType { STATEMENT_INSERT, STATEMENT_SELECT };
struct Statement {
    StatementType type;
    Row row_to_insert;
};

void do_meta_command(const std::string& command, Table* table) {
    if (command == ".exit") {
        db_close(table);
        std::cout << "Bye!" << std::endl;
        exit(EXIT_SUCCESS);
    } else {
        std::cout << "Unrecognized command '" << command << "'" << std::endl;
    }
}

bool prepare_statement(const std::string& input, Statement* statement) {
    if (input.rfind("insert", 0) == 0) {
        statement->type = STATEMENT_INSERT;
        int args_assigned = sscanf(input.c_str(), "insert %u %s %s",
                                   &statement->row_to_insert.id,
                                   statement->row_to_insert.username,
                                   statement->row_to_insert.email);

        if (args_assigned < 3) {
            std::cout << "Syntax error. Could not parse statement." << std::endl;
            return false;
        }
        return true;
    }
    if (input == "select") {
        statement->type = STATEMENT_SELECT;
        return true;
    }
    std::cout << "Unrecognized keyword at start of '" << input << "'." << std::endl;
    return false;
}

// --- Statement Execution ---
void execute_insert(Statement* statement, Table* table) {
    if (table->num_rows >= TABLE_MAX_ROWS) {
        std::cout << "Error: Table full." << std::endl;
        return;
    }

    Row* row_to_insert = &(statement->row_to_insert);
    Cursor* cursor = table_end(table);
    
    serialize_row(row_to_insert, cursor_value(cursor));
    table->num_rows += 1;
    
    delete cursor;
    std::cout << "Executed." << std::endl;
}

void execute_select(Statement* statement, Table* table) {
    Cursor* cursor = table_start(table);
    Row row;

    while (!(cursor->end_of_table)) {
        deserialize_row(cursor_value(cursor), &row);
        print_row(row);
        cursor_advance(cursor);
    }

    delete cursor;
    std::cout << "Executed." << std::endl;
}

void execute_statement(Statement* statement, Table* table) {
    switch (statement->type) {
        case STATEMENT_INSERT:
            execute_insert(statement, table);
            break;
        case STATEMENT_SELECT:
            execute_select(statement, table);
            break;
    }
}

// --- Main REPL Loop ---
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Must supply a database filename." << std::endl;
        exit(EXIT_FAILURE);
    }
    
    std::string filename = argv[1];
    Table* table = db_open(filename);

    std::string input_line;
    while (true) {
        std::cout << "db > ";
        std::getline(std::cin, input_line);

        if (input_line.empty()) {
            continue;
        }

        if (input_line[0] == '.') {
            do_meta_command(input_line, table);
            continue;
        }

        Statement statement;
        if (prepare_statement(input_line, &statement)) {
            execute_statement(&statement, table);
        }
    }

    return 0;
}
