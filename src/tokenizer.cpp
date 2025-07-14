#include "tokenizer.h"
#include <cctype>
#include <algorithm>

Tokenizer::Tokenizer(const std::string& sql) : input(sql), position(0) {}

Token Tokenizer::next_token() {
    while (position < input.length() && isspace(input[position])) {
        position++;
    }
    if (position >= input.length()) {
        return {TokenType::END_OF_FILE, ""};
    }
    char current_char = input[position];
    if (isalpha(current_char)) return keyword_or_identifier();
    if (isdigit(current_char)) return number();
    if (current_char == '\'' || current_char == '"') return string();
    if (ispunct(current_char)) return symbol();
    return {TokenType::UNKNOWN, std::string(1, current_char)};
}

Token Tokenizer::keyword_or_identifier() {
    size_t start = position;
    while (position < input.length() && (isalnum(input[position]) || input[position] == '_')) {
        position++;
    }
    std::string text = input.substr(start, position - start);
    std::string upper_text = text;
    std::transform(upper_text.begin(), upper_text.end(), upper_text.begin(), ::toupper);

    if (upper_text == "INSERT" || upper_text == "SELECT" || upper_text == "DELETE" || upper_text == "FROM" || upper_text == "INTO" || upper_text == "VALUES") {
        return {TokenType::KEYWORD, upper_text};
    }
    return {TokenType::IDENTIFIER, text};
}

Token Tokenizer::number() {
    size_t start = position;
    while (position < input.length() && isdigit(input[position])) {
        position++;
    }
    return {TokenType::NUMBER, input.substr(start, position - start)};
}

Token Tokenizer::string() {
    char quote_char = input[position++];
    size_t start = position;
    while (position < input.length() && input[position] != quote_char) {
        position++;
    }
    std::string text = input.substr(start, position - start);
    if (position < input.length()) {
        position++;
    }
    return {TokenType::STRING, text};
}

Token Tokenizer::symbol() {
    return {TokenType::SYMBOL, std::string(1, input[position++])};
}
