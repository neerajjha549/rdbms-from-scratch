#ifndef PARSER_H
#define PARSER_H

#include "tokenizer.h"
#include "ast.h"

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    std::unique_ptr<AstNode> parse();
private:
    const std::vector<Token>& tokens;
    size_t current_token_pos = 0;
    std::unique_ptr<AstNode> parse_insert();
    std::unique_ptr<AstNode> parse_select();
    std::unique_ptr<AstNode> parse_delete();
    const Token& current_token();
    void advance_token();
    void expect_token(TokenType type, const std::string& error_message);
};

#endif // PARSER_H
