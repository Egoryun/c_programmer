#ifndef PARSER_H
#define PARSER_H

#include "lexer.h" // For Token and TokenType

// --- AST Node Types ---
typedef enum {
    AST_NODE_PROGRAM,
    AST_NODE_FUNCTION_DECLARATION,
    AST_NODE_RETURN_STATEMENT,
    AST_NODE_INTEGER_LITERAL,
    AST_NODE_BINARY_OPERATION // New
    // Add more types as the language grows
} ASTNodeType;

// --- Base AST Node Structure ---
// All AST nodes will have this as their first member for polymorphism
typedef struct ASTNode {
    ASTNodeType type;
} ASTNode;

// --- Specific AST Node Structures ---

// Integer Literal: e.g., 42
typedef struct {
    ASTNode base;
    int value;
    Token token_literal; // Store the original token for location/debugging
} IntegerLiteralNode;

// Binary Operation: e.g., 1 + 2
typedef struct BinaryOperationNode {
    ASTNode base;                // type will be AST_NODE_BINARY_OPERATION
    struct ASTNode* left;        // Left-hand side expression
    TokenType operator_token_type; // Type of the operator (e.g., TOKEN_PLUS)
    struct ASTNode* right;       // Right-hand side expression
} BinaryOperationNode;

// Return Statement: e.g., return 42;
typedef struct {
    ASTNode base;
    ASTNode* expression; // The expression being returned
} ReturnStatementNode;

// Function Body: A sequence of statements
typedef struct {
    ASTNode base;
    ASTNode** statements;     // Array of statement ASTNode pointers
    int statement_count;      // Number of statements in the array
    int capacity;             // Allocated capacity of the statements array
} FunctionBodyNode;

// Function Declaration: e.g., int main() { ... }
typedef struct {
    ASTNode base;
    Token function_name; // e.g., "main"
    FunctionBodyNode* body;
} FunctionDeclarationNode;

// Program Node: The root of the AST
// For this minimal compiler, a program is just one function declaration (main)
typedef struct {
    ASTNode base;
    FunctionDeclarationNode* function_declaration;
} ProgramNode;


// --- Parser Function Prototypes ---

// Main parsing function
ProgramNode* parse(const char* source_code);

// --- AST Memory Management Function Prototypes ---
void free_ast_node(ASTNode* node); // General function to free any AST node
void free_program_node(ProgramNode* program_node);

// --- Helper Function Prototypes for Parser (typically in parser.c but declared here if needed by other modules) ---
// These might be static in parser.c if not needed externally.

// Function prototype for resetting lexer state (defined in lexer.c)
void reset_lexer_state();


#endif // PARSER_H
