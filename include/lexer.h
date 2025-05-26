#ifndef LEXER_H
#define LEXER_H

// Define Token Types
typedef enum {
    TOKEN_INT,             // int
    TOKEN_RETURN,          // return
    TOKEN_IDENTIFIER,      // variable_name, function_name
    TOKEN_INTEGER_LITERAL, // 123, 0, 42
    TOKEN_LPAREN,          // (
    TOKEN_RPAREN,          // )
    TOKEN_LBRACE,          // {
    TOKEN_RBRACE,          // }
    TOKEN_SEMICOLON,       // ;
    TOKEN_PLUS,            // +
    TOKEN_MINUS,           // -
    TOKEN_STAR,            // *
    TOKEN_SLASH,           // /
    TOKEN_EOF,             // End of Input
    TOKEN_UNKNOWN          // Unrecognized character
} TokenType;

// Lexer Token Structure
typedef struct {
    TokenType type;
    const char* lexeme_start; // Pointer to the start of the lexeme in the source
    int lexeme_length;        // Length of the lexeme
    int line;                 // Line number where the token appears
    int column;               // Column number where the token begins
} Token;

// Function prototype for the lexer
// Takes a pointer to a pointer to the current position in the source code.
// The inner pointer is updated by the function to point past the consumed token.
Token get_next_token(const char** source_code_pointer);

// Helper function to print tokens (for debugging)
// void print_token(Token token); // Declaration moved to lexer.c to make it static, not part of public API

#endif // LEXER_H
