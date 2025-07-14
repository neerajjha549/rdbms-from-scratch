#ifndef AST_H
#define AST_H

#include "common.h"
#include "row.h"

enum class AstNodeType {
    INSERT_STATEMENT,
    SELECT_STATEMENT,
    DELETE_STATEMENT
};

struct AstNode {
    virtual ~AstNode() = default;
    virtual AstNodeType type() const = 0;
};

struct InsertStatementNode : public AstNode {
    Row row_to_insert;
    AstNodeType type() const override { return AstNodeType::INSERT_STATEMENT; }
};

struct SelectStatementNode : public AstNode {
    AstNodeType type() const override { return AstNodeType::SELECT_STATEMENT; }
};

struct DeleteStatementNode : public AstNode {
    uint32_t id_to_delete;
    AstNodeType type() const override { return AstNodeType::DELETE_STATEMENT; }
};

#endif // AST_H
