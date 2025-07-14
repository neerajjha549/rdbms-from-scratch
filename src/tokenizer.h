#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "common.h"

enum class TokenType {
    KEYWORD, IDENTIFIER, NUMBER, STRING, SYMBOL, END_OF_FILE, UNKNOWN
};

struct Token {
    TokenType type;
    std::string text;
};

class Tokenizer {
public:
    Tokenizer(const std::string& sql);
    Token next_token();
private:
    std::string input;
    size_t position;
    Token keyword_or_identifier();
    Token number();
    Token string();
    Token symbol();
};

#endif // TOKENIZER_H
