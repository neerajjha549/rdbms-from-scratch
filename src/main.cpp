#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

#include "common.h"
#include "row.h"
#include "pager.h"
#include "table.h"
#include "btree.h"


enum StatementType { STATEMENT_INSERT, STATEMENT_SELECT };
struct Statement {
    StatementType type;
    Row row_to_insert;
};

void print_row(const Row& row) {
    std::cout << "(" << row.id << ", " << row.username << ", " << row.email << ")" << std::endl;
}

void print_constants() {
    std::cout << "ROW_SIZE: " << ROW_SIZE << std::endl;
    std::cout << "COMMON_NODE_HEADER_SIZE: " << COMMON_NODE_HEADER_SIZE << std::endl;
    std::cout << "LEAF_NODE_HEADER_SIZE: " << LEAF_NODE_HEADER_SIZE << std::endl;
    std::cout << "LEAF_NODE_CELL_SIZE: " << LEAF_NODE_CELL_SIZE << std::endl;
    std::cout << "LEAF_NODE_SPACE_FOR_CELLS: " << LEAF_NODE_SPACE_FOR_CELLS << std::endl;
    std::cout << "LEAF_NODE_MAX_CELLS: " << LEAF_NODE_MAX_CELLS << std::endl;
}


void do_meta_command(const std::string& command, Table* table) {
    if (command == ".exit") {
        db_close(table);
        std::cout << "Bye!" << std::endl;
        exit(EXIT_SUCCESS);
    } else if (command == ".constants") {
        std::cout << "Constants:" << std::endl;
        print_constants();
    } else if (command == ".btree") {
        std::cout << "Tree:" << std::endl;
        print_tree(table->pager, 0, 0);
    } else {
        std::cout << "Unrecognized command '" << command << "'" << std::endl;
    }
}

bool prepare_statement(const std::string& input, Statement* statement) {
    if (input.rfind("insert", 0) == 0) {
        statement->type = STATEMENT_INSERT;
        char username[COLUMN_USERNAME_SIZE + 1];
        char email[COLUMN_EMAIL_SIZE + 1];
        int args_assigned = sscanf(input.c_str(), "insert %u %s %s",
                                      &statement->row_to_insert.id,
                                      username,
                                      email);

        if (args_assigned < 3) {
            std::cout << "Syntax error. Usage: insert <id> <username> <email>" << std::endl;
            return false;
        }
        username[COLUMN_USERNAME_SIZE] = '\0';
        email[COLUMN_EMAIL_SIZE] = '\0';
        strncpy(statement->row_to_insert.username, username, COLUMN_USERNAME_SIZE);
        strncpy(statement->row_to_insert.email, email, COLUMN_EMAIL_SIZE);
        
        return true;
    }
    if (input == "select") {
        statement->type = STATEMENT_SELECT;
        return true;
    }
    std::cout << "Unrecognized keyword at start of '" << input << "'." << std::endl;
    return false;
}

void execute_insert(Statement* statement, Table* table) {
    Row* row_to_insert = &(statement->row_to_insert);
    uint32_t key_to_insert = row_to_insert->id;
    Cursor* cursor = table_find(table, key_to_insert);

    void* node = get_page(table->pager, cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);

    if (cursor->cell_num < num_cells) {
        uint32_t key_at_index = *leaf_node_key(node, cursor->cell_num);
        if (key_at_index == key_to_insert) {
            std::cout << "Error: Duplicate key." << std::endl;
            delete cursor;
            return;
        }
    }

    leaf_node_insert(cursor, row_to_insert->id, row_to_insert);
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