#include "common.h"
#include "row.h"
#include "table.h"
#include "btree.h"

enum StatementType { STATEMENT_INSERT, STATEMENT_SELECT, STATEMENT_DELETE };

struct Statement {
    StatementType type;
    Row row_to_insert;
    uint32_t id_to_delete;
};

void print_prompt() {
    std::cout << "db > ";
}

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
    std::cout << "INTERNAL_NODE_HEADER_SIZE: " << INTERNAL_NODE_HEADER_SIZE << std::endl;
    std::cout << "INTERNAL_NODE_CELL_SIZE: " << INTERNAL_NODE_CELL_SIZE << std::endl;
    std::cout << "INTERNAL_NODE_MAX_CELLS: " << INTERNAL_NODE_MAX_CELLS << std::endl;
}

void do_meta_command(const std::string& command, Table* table) {
    if (command == ".exit") {
        db_close(table);
        std::cout << "Bye!" << std::endl;
        exit(EXIT_SUCCESS);
    } else if (command == ".btree") {
        std::cout << "Tree:" << std::endl;
        print_tree(table->pager, table->root_page_num, 0);
    } else if (command == ".constants") {
        std::cout << "Constants:" << std::endl;
        print_constants();
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

void execute_statement(Statement* statement, Table* table) {
    switch (statement->type) {
        case STATEMENT_INSERT:
            table_insert(table, &(statement->row_to_insert));
            break;
        case STATEMENT_SELECT:
            {
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
            break;
        case STATEMENT_DELETE:
            table_delete(table, statement->id_to_delete);
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
