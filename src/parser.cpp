#include "parser.h"

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

std::unique_ptr<AstNode> Parser::parse() {
    if (current_token().type == TokenType::KEYWORD) {
        if (current_token().text == "INSERT") return parse_insert();
        if (current_token().text == "SELECT") return parse_select();
        if (current_token().text == "DELETE") return parse_delete();
    }
    throw std::runtime_error("Syntax error: Unexpected keyword " + current_token().text);
}

std::unique_ptr<AstNode> Parser::parse_insert() {
    auto node = std::unique_ptr<InsertStatementNode>(new InsertStatementNode());
    advance_token(); // Consume 'INSERT'
    expect_token(TokenType::NUMBER, "Expected ID after INSERT.");
    node->row_to_insert.id = std::stoul(current_token().text);
    advance_token();
    expect_token(TokenType::IDENTIFIER, "Expected username after ID.");
    strncpy(node->row_to_insert.username, current_token().text.c_str(), COLUMN_USERNAME_SIZE);
    advance_token();
    expect_token(TokenType::IDENTIFIER, "Expected email after username.");
    strncpy(node->row_to_insert.email, current_token().text.c_str(), COLUMN_EMAIL_SIZE);
    advance_token();
    return node;
}

std::unique_ptr<AstNode> Parser::parse_select() {
    advance_token(); // Consume 'SELECT'
    return std::unique_ptr<SelectStatementNode>(new SelectStatementNode());
}

std::unique_ptr<AstNode> Parser::parse_delete() {
    auto node = std::unique_ptr<DeleteStatementNode>(new DeleteStatementNode());
    advance_token(); // Consume 'DELETE'
    expect_token(TokenType::NUMBER, "Expected ID after DELETE.");
    node->id_to_delete = std::stoul(current_token().text);
    advance_token();
    return node;
}

const Token& Parser::current_token() {
    if (current_token_pos >= tokens.size()) {
        throw std::runtime_error("Unexpected end of input.");
    }
    return tokens[current_token_pos];
}

void Parser::advance_token() {
    if (current_token_pos < tokens.size()) {
        current_token_pos++;
    }
}

void Parser::expect_token(TokenType type, const std::string& error_message) {
    if (current_token().type != type) {
        throw std::runtime_error("Syntax Error: " + error_message);
    }
}
