#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"        // For Token, TokenType
#include "symbol_table.h" // For SymbolTable
#include <stdbool.h>

// --- Abstract Syntax Tree (AST) Node Types ---
typedef enum {
    AST_NODE_PROGRAM,
    AST_NODE_FUNCTION_DECLARATION,
    AST_NODE_FUNCTION_BODY,       // Represents a block of statements
    AST_NODE_VARIABLE_DECLARATION,
    AST_NODE_ASSIGNMENT_STATEMENT,
    AST_NODE_VARIABLE_USAGE,
    AST_NODE_RETURN_STATEMENT,
    AST_NODE_IF_STATEMENT,
    AST_NODE_INTEGER_LITERAL,
    AST_NODE_BINARY_OPERATION,
    AST_NODE_FUNCTION_CALL      // New
} ASTNodeType;

// Base AST Node Structure (common to all specific node types)
typedef struct ASTNode {
    ASTNodeType type;
} ASTNode;

// --- Specific AST Node Structures ---

typedef struct IntegerLiteralNode {
    ASTNode base;
    Token token_literal; // The original token for the integer
    int value;
} IntegerLiteralNode;

typedef struct BinaryOperationNode {
    ASTNode base;
    struct ASTNode* left;
    TokenType operator_token_type; // e.g., TOKEN_PLUS, TOKEN_EQ_EQ
    struct ASTNode* right;
} BinaryOperationNode;

typedef struct ReturnStatementNode {
    ASTNode base;
    struct ASTNode* expression;
} ReturnStatementNode;

typedef struct VariableDeclarationNode {
    ASTNode base;
    Token type_token;       // e.g., the TOKEN_INT token
    char* variable_name;    // strdup'd
} VariableDeclarationNode;

typedef struct AssignmentStatementNode {
    ASTNode base;
    char* variable_name;    // strdup'd name of variable being assigned to
    struct ASTNode* expression; // RHS expression
} AssignmentStatementNode;

typedef struct VariableUsageNode {
    ASTNode base;
    char* variable_name;    // strdup'd name of variable being used
} VariableUsageNode;

// Represents a block of statements, e.g., function body or if/else block body
typedef struct FunctionBodyNode {
    ASTNode base; // type will be AST_NODE_FUNCTION_BODY
    struct ASTNode** statements;
    int statement_count;
    int capacity;
} FunctionBodyNode;

typedef struct ParameterNode {
    Token type_token; 
    Token name_token; 
} ParameterNode;

typedef struct FunctionDeclarationNode {
    ASTNode base;
    Token function_name_token;
    SymbolTable symbol_table;      // Symbol table for this function's scope
    struct ParameterNode* parameters;
    int num_parameters;
    int capacity_parameters;
    struct FunctionBodyNode* body;
} FunctionDeclarationNode;

typedef struct IfStatementNode {
    ASTNode base;
    struct ASTNode* condition;
    struct FunctionBodyNode* then_block;
    struct FunctionBodyNode* else_block; // Can be NULL
} IfStatementNode;

// New Node for Function Calls
typedef struct FunctionCallNode {
    ASTNode base;
    char* function_name;          // strdup'd name of the function being called
    struct ASTNode** arguments;   // Dynamically allocated array of argument expressions
    int num_arguments;
    int capacity_arguments;       // Current capacity of the arguments array
} FunctionCallNode;

typedef struct ProgramNode {
    ASTNode base;
    struct FunctionDeclarationNode** functions;
    int num_functions;
    int capacity_functions;
} ProgramNode;


// --- Main Parser Function ---
ProgramNode* parse(const char* source_code);

// --- AST Freeing Function ---
void free_program_node(ProgramNode* program_node); // Frees the entire AST
void free_ast_node(ASTNode* node); // Helper, usually static in parser.c but prototype if needed elsewhere

#endif // PARSER_H
