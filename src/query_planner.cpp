#include "query_planner.h"

std::vector<Bytecode> QueryPlanner::compile(AstNode* root) {
    std::vector<Bytecode> program;
    if (!root) {
        return program;
    }

    switch (root->type()) {
        case AstNodeType::INSERT_STATEMENT:
            program.push_back({OpCode::EXECUTE_INSERT});
            break;
        case AstNodeType::SELECT_STATEMENT:
            program.push_back({OpCode::EXECUTE_SELECT});
            break;
        case AstNodeType::DELETE_STATEMENT:
            program.push_back({OpCode::EXECUTE_DELETE});
            break;
    }
    program.push_back({OpCode::HALT});
    return program;
}
