#include "lexer.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h> // For print_token

// Helper to create a token
static Token make_token(TokenType type, const char* start, int length, int line, int column) {
    Token token;
    token.type = type;
    token.lexeme_start = start;
    token.lexeme_length = length;
    token.line = line;
    token.column = column;
    return token;
}

// Helper to check if a character can be part of an identifier (after the first char)
static int is_identifier_char(char c) {
    return isalnum(c) || c == '_';
}

// Globals to keep track of the current line and column number
static int current_line = 1;
static int current_column = 1; // Start at column 1

// Call this when starting to parse a new source string to reset lexer state
void reset_lexer_state() {
    current_line = 1;
    current_column = 1;
}

Token get_next_token(const char** source_code_pointer) {
    const char* current = *source_code_pointer;

    while (1) {
        const char* start_of_lexeme = current;
        int token_start_column = current_column;

        switch (*current) {
            case '\0':
                *source_code_pointer = current;
                return make_token(TOKEN_EOF, start_of_lexeme, 0, current_line, token_start_column);

            case ' ':
            case '\r':
            case '\t':
                current++;
                current_column++;
                continue; // Skip whitespace

            case '\n':
                current++;
                current_line++;
                current_column = 1; // Reset column to 1
                continue; // Skip newline, increment line counter

            case '(':
                current++;
                *source_code_pointer = current;
                current_column++;
                return make_token(TOKEN_LPAREN, start_of_lexeme, 1, current_line, token_start_column);
            case ')':
                current++;
                *source_code_pointer = current;
                current_column++;
                return make_token(TOKEN_RPAREN, start_of_lexeme, 1, current_line, token_start_column);
            case '{':
                current++;
                *source_code_pointer = current;
                current_column++;
                return make_token(TOKEN_LBRACE, start_of_lexeme, 1, current_line, token_start_column);
            case '}':
                current++;
                *source_code_pointer = current;
                current_column++;
                return make_token(TOKEN_RBRACE, start_of_lexeme, 1, current_line, token_start_column);
            case ';':
                current++;
                *source_code_pointer = current;
                current_column++;
                return make_token(TOKEN_SEMICOLON, start_of_lexeme, 1, current_line, token_start_column);
            case '+':
                current++;
                *source_code_pointer = current;
                current_column++;
                return make_token(TOKEN_PLUS, start_of_lexeme, 1, current_line, token_start_column);

            default:
                if (isalpha(*current) || *current == '_') { // Identifiers or keywords
                    const char* ident_start = current;
                    current++;
                    current_column++;
                    while (is_identifier_char(*current)) {
                        current++;
                        current_column++;
                    }
                    int length = current - ident_start;
                    *source_code_pointer = current;

                    // Check for keywords
                    if (length == 3 && strncmp(ident_start, "int", 3) == 0) {
                        return make_token(TOKEN_INT, ident_start, length, current_line, token_start_column);
                    }
                    if (length == 6 && strncmp(ident_start, "return", 6) == 0) {
                        return make_token(TOKEN_RETURN, ident_start, length, current_line, token_start_column);
                    }
                    return make_token(TOKEN_IDENTIFIER, ident_start, length, current_line, token_start_column);
                } else if (isdigit(*current)) { // Integer literals
                    const char* num_start = current;
                    current++;
                    current_column++;
                    while (isdigit(*current)) {
                        current++;
                        current_column++;
                    }
                    *source_code_pointer = current;
                    return make_token(TOKEN_INTEGER_LITERAL, num_start, current - num_start, current_line, token_start_column);
                } else {
                    // Unknown token
                    current++;
                    *source_code_pointer = current;
                    current_column++;
                    return make_token(TOKEN_UNKNOWN, start_of_lexeme, 1, current_line, token_start_column);
                }
        }
    }
}

// Helper function to print tokens (for debugging) - now static
static void print_token(Token token) {
    printf("Token Type: %d, Lexeme: %.*s, Line: %d, Column: %d\n",
           token.type, token.lexeme_length, token.lexeme_start, token.line, token.column);
}
