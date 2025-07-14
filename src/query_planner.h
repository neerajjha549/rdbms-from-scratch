#ifndef QUERY_PLANNER_H
#define QUERY_PLANNER_H

#include "ast.h"
#include "vm.h"

class QueryPlanner {
public:
    static std::vector<Bytecode> compile(AstNode* root);
};

#endif // QUERY_PLANNER_H
