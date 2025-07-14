#include "common.h"
#include "row.h"
#include "table.h"
#include "btree.h"
#include "tokenizer.h"
#include "parser.h"
#include "ast.h"
#include "query_planner.h"
#include "vm.h"

void print_prompt() { std::cout << "db > "; }

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

// Prepares the statement by running it through the tokenizer and parser.
std::unique_ptr<AstNode> prepare_statement(const std::string& input) {
    try {
        Tokenizer tokenizer(input);
        std::vector<Token> tokens;
        Token token;
        do {
            token = tokenizer.next_token();
            if (token.type != TokenType::END_OF_FILE) {
                 tokens.push_back(token);
            }
        } while (token.type != TokenType::END_OF_FILE);

        if (tokens.empty()) {
            return nullptr; // Not a statement, just empty input
        }

        Parser parser(tokens);
        return parser.parse();

    } catch (const std::runtime_error& e) {
        std::cout << e.what() << std::endl;
        return nullptr;
    }
}

// Executes the statement by compiling the AST and running it on the VM.
void execute_statement(std::unique_ptr<AstNode>& root_node, Table* table) {
    if (!root_node) {
        return; // Empty statement
    }

    try {
        VirtualMachine vm(table);
        std::vector<Bytecode> program = QueryPlanner::compile(root_node.get());

        // Before execution, push operands onto the VM's stack based on the AST node type.
        // This is the bridge between the AST (data) and the VM (logic).
        switch (root_node->type()) {
            case AstNodeType::INSERT_STATEMENT: {
                auto* node = static_cast<InsertStatementNode*>(root_node.get());
                vm.push_row(node->row_to_insert);
                break;
            }
            case AstNodeType::DELETE_STATEMENT: {
                auto* node = static_cast<DeleteStatementNode*>(root_node.get());
                vm.push_int(node->id_to_delete);
                break;
            }
            case AstNodeType::SELECT_STATEMENT:
                // No operands to push
                break;
        }

        vm.execute(program);
    } catch(const std::runtime_error& e) {
        std::cout << "Execution Error: " << e.what() << std::endl;
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

        std::unique_ptr<AstNode> ast_root = prepare_statement(input_line);
        if (ast_root) {
            execute_statement(ast_root, table);
        }
    }
    return 0;
}
