#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <cstdio>

#include "table.h"
#include "btree.h"
#include "row.h"

enum StatementType { STATEMENT_INSERT, STATEMENT_SELECT, STATEMENT_DELETE };

struct Statement {
    StatementType type;
    Row row_to_insert; // only used by insert statement
    uint32_t id_to_delete; // only used by delete statement
};

void print_prompt() {
    std::cout << "db > ";
}

void print_row(const Row& row) {
    std::cout << "(" << row.id << ", " << row.username << ", " << row.email << ")" << std::endl;
}

void do_meta_command(const std::string& command, Table* table) {
    if (command == ".exit") {
        db_close(table);
        std::cout << "Bye!" << std::endl;
        exit(EXIT_SUCCESS);
    } else if (command == ".constants") {
        std::cout << "Constants:" << std::endl;
        // You would need to implement print_constants() if you want to use it
        // print_constants();
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
    if (input.rfind("delete", 0) == 0) {
        statement->type = STATEMENT_DELETE;
        int args_assigned = sscanf(input.c_str(), "delete %u", &statement->id_to_delete);
        if (args_assigned < 1) {
            std::cout << "Syntax error. Must provide an ID to delete." << std::endl;
            return false;
        }
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
    std::cout << "Executed." << std::endl;
}

void execute_delete(Statement* statement, Table* table) {
    uint32_t key_to_delete = statement->id_to_delete;
    Cursor* cursor = table_find(table, key_to_delete);
    void* node = get_page(table->pager, cursor->page_num);

    if (*leaf_node_num_cells(node) > cursor->cell_num && *leaf_node_key(node, cursor->cell_num) == key_to_delete) {
        leaf_node_delete(cursor);
        std::cout << "Executed." << std::endl;
    } else {
        std::cout << "Error: Key " << key_to_delete << " not found." << std::endl;
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
        case STATEMENT_DELETE:
            execute_delete(statement, table);
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
        print_prompt();
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