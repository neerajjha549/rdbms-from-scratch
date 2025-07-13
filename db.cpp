#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

// Define fixed-size constants for our table schema.
// This simplifies serialization.
#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

// Row structure representing a single record in our 'users' table.
struct Row {
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE + 1]; // +1 for null terminator
    char email[COLUMN_EMAIL_SIZE + 1];   // +1 for null terminator
};

// Define the size of each part of the serialized row.
const uint32_t ID_SIZE = sizeof(uint32_t);
const uint32_t USERNAME_SIZE = sizeof(char) * (COLUMN_USERNAME_SIZE + 1);
const uint32_t EMAIL_SIZE = sizeof(char) * (COLUMN_EMAIL_SIZE + 1);
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

// Define the layout of the serialized row.
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;

// --- Serialization & Deserialization ---
// These functions convert the Row struct to and from a compact binary format
// that can be written to disk.

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

// --- Pager & Table Abstraction (Simplified) ---
// For now, our "pager" is just a file pointer, and our "table" manages
// the file and the number of rows.

const uint32_t PAGE_SIZE = 4096;
const uint32_t TABLE_MAX_PAGES = 100;
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

struct Table {
    FILE* file_pointer;
    uint32_t num_rows;
};

// --- Function to open the database file ---
Table* db_open(const std::string& filename) {
    Table* table = new Table();
    table->file_pointer = fopen(filename.c_str(), "r+");

    // If the file doesn't exist, create it.
    if (table->file_pointer == nullptr) {
        table->file_pointer = fopen(filename.c_str(), "w+");
        if (table->file_pointer == nullptr) {
            std::cerr << "Error: Could not open or create file '" << filename << "'." << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    // Get the file length to calculate the number of rows.
    fseek(table->file_pointer, 0, SEEK_END);
    long file_size = ftell(table->file_pointer);
    table->num_rows = file_size / ROW_SIZE;

    return table;
}

// --- Function to close the database file ---
void db_close(Table* table) {
    fclose(table->file_pointer);
    delete table;
}

// --- Function to get the memory location for a specific row ---
void* row_slot(Table* table, uint32_t row_num) {
    // For now, we are just using a simple static buffer.
    // In the future, this will be replaced by a proper page cache.
    static char page_buffer[PAGE_SIZE];

    uint32_t page_num = row_num / ROWS_PER_PAGE;
    // For now, we only support a single page for simplicity.
    if (page_num > 0) {
        std::cout << "Need to implement reading from multiple pages." << std::endl;
        exit(EXIT_FAILURE);
    }
    
    // Seek to the correct position and read the page
    fseek(table->file_pointer, page_num * PAGE_SIZE, SEEK_SET);
    fread(page_buffer, PAGE_SIZE, 1, table->file_pointer);

    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    uint32_t byte_offset = row_offset * ROW_SIZE;
    return page_buffer + byte_offset;
}

void print_row(const Row& row) {
    std::cout << "(" << row.id << ", " << row.username << ", " << row.email << ")" << std::endl;
}

// --- Statement & REPL ---
// These structures and functions handle parsing and executing user commands.

enum StatementType { STATEMENT_INSERT, STATEMENT_SELECT };

struct Statement {
    StatementType type;
    Row row_to_insert; // Only used by insert statements
};

// --- Meta Commands (like .exit) ---
void do_meta_command(const std::string& command, Table* table) {
    if (command == ".exit") {
        db_close(table);
        std::cout << "Bye!" << std::endl;
        exit(EXIT_SUCCESS);
    } else {
        std::cout << "Unrecognized command '" << command << "'" << std::endl;
    }
}

// --- SQL Command Preparation (Parsing) ---
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
    
    // Serialize the row and write it to the end of the file
    char buffer[ROW_SIZE];
    serialize_row(row_to_insert, buffer);
    
    fseek(table->file_pointer, table->num_rows * ROW_SIZE, SEEK_SET);
    fwrite(buffer, ROW_SIZE, 1, table->file_pointer);
    
    table->num_rows += 1;
    
    std::cout << "Executed." << std::endl;
}

void execute_select(Statement* statement, Table* table) {
    Row row;
    char buffer[ROW_SIZE];

    for (uint32_t i = 0; i < table->num_rows; i++) {
        fseek(table->file_pointer, i * ROW_SIZE, SEEK_SET);
        fread(buffer, ROW_SIZE, 1, table->file_pointer);
        deserialize_row(buffer, &row);
        print_row(row);
    }
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
