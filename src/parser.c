#include "parser.h"
#include "lexer.h" // Already included in parser.h, but good for clarity
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For strncmp

// --- Global variables for parser state ---
static const char* current_source_ptr; // Pointer to the current position in the source code
static Token current_token;          // The current token being processed
static int parser_error_occurred = 0; // Flag to track if an error has occurred

// --- Forward declarations for recursive descent functions ---
static ProgramNode* parse_program();
static FunctionDeclarationNode* parse_function_declaration();
static FunctionBodyNode* parse_function_body(SymbolTable* current_st); // Modified
static ASTNode* parse_statement(SymbolTable* current_st);             // Modified
static ASTNode* parse_variable_declaration_statement(SymbolTable* current_st);
static ReturnStatementNode* parse_return_statement(SymbolTable* current_st); 
static ASTNode* parse_expression(SymbolTable* current_st);                        // New top-level
static ASTNode* parse_comparison_expression(SymbolTable* current_st);
static ASTNode* parse_additive_expression(SymbolTable* current_st);
static ASTNode* parse_multiplicative_expression(SymbolTable* current_st);
static ASTNode* parse_primary_expression(SymbolTable* current_st);
static IntegerLiteralNode* parse_integer_literal();
static ASTNode* parse_if_statement(SymbolTable* current_st); // New
static FunctionBodyNode* parse_block_statement(SymbolTable* current_st); // New

// --- AST Node Creation Helper Prototypes (if not already above) ---
static ASTNode* create_assignment_statement_node(Token identifier_token, ASTNode* expression_node);
static ASTNode* create_variable_usage_node(Token identifier_token);
static ASTNode* create_if_statement_node(ASTNode* condition, FunctionBodyNode* then_block, FunctionBodyNode* else_block); // New


// --- Helper Function Implementations ---

// Advance to the next token
static void advance_token() {
    if (current_token.type != TOKEN_EOF && !parser_error_occurred) {
        current_token = get_next_token(&current_source_ptr);
    }
}

// Report a parsing error
static void report_error(const char* message, Token token) {
    // Use token.line and token.column
    fprintf(stderr, "Error: %s at line %d, column %d. ", message, token.line, token.column);
    if (token.type == TOKEN_EOF) {
        fprintf(stderr, "Unexpected end of input.\n");
    } else {
        fprintf(stderr, "Got token '%.*s' (Type: %d).\n", token.lexeme_length, token.lexeme_start, token.type);
    }
    parser_error_occurred = 1;
}

// Consume the current token if it matches the expected type, otherwise report an error
static Token eat_token(TokenType expected_type) {
    Token consumed_token = current_token; // Save for return, even if error occurs
    if (parser_error_occurred) {
        // If an error already occurred, don't try to eat, return the current (potentially problematic) token
        // or a dummy. Current token is better for context if error cascades.
        return current_token;
    }
    if (current_token.type == expected_type) {
        advance_token();
    } else {
        char error_msg[128];
        sprintf(error_msg, "Expected token type %d ('%s') but got %d ('%.*s')",
                expected_type, "TODO_token_type_to_string(expected_type)", // Placeholder for token type to string
                current_token.type, current_token.lexeme_length, current_token.lexeme_start);
        report_error(error_msg, current_token);
    }
    return consumed_token;
}

// Peek at the current token without consuming it
static Token peek_token() {
    return current_token;
}

// --- AST Node Creation Functions ---

static IntegerLiteralNode* create_integer_literal_node(Token token) {
    IntegerLiteralNode* node = (IntegerLiteralNode*)malloc(sizeof(IntegerLiteralNode));
    if (!node) {
        report_error("Memory allocation failed for IntegerLiteralNode", token); // Pass the original token
        return NULL;
    }
    node->base.type = AST_NODE_INTEGER_LITERAL;
    node->token_literal = token; // Store the original token

    // Convert lexeme to integer
    char* lexeme_str = (char*)malloc(token.lexeme_length + 1);
    if (!lexeme_str) {
        report_error("Memory allocation failed for lexeme string", token);
        free(node);
        return NULL;
    }
    strncpy(lexeme_str, token.lexeme_start, token.lexeme_length);
    lexeme_str[token.lexeme_length] = '\0';
    node->value = atoi(lexeme_str);
    free(lexeme_str);
    return node;
}

static ReturnStatementNode* create_return_statement_node(ASTNode* expression) {
    ReturnStatementNode* node = (ReturnStatementNode*)malloc(sizeof(ReturnStatementNode));
    if (!node) {
        // Error reporting would happen in the caller or deeper if expression is NULL
        // For now, assume expression is valid or its creation handled errors
        return NULL;
    }
    node->base.type = AST_NODE_RETURN_STATEMENT;
    node->expression = expression;
    return node;
}

static FunctionBodyNode* create_function_body_node() {
    FunctionBodyNode* node = (FunctionBodyNode*)malloc(sizeof(FunctionBodyNode));
    if (!node) {
        // No token available here for report_error, critical failure
        fprintf(stderr, "Critical: Memory allocation failed for FunctionBodyNode.\n");
        parser_error_occurred = 1; // Set error flag
        return NULL;
    }
    node->base.type = AST_NODE_FUNCTION_BODY; // Corrected type
                                                 // It should be a distinct type or part of FunctionDeclaration.
                                                 // Let's assume FunctionBodyNode has its own type or this is a typo.
                                                 // For now, let's give it a placeholder type if one isn't defined,
                                                 // or ensure parser.h defines AST_NODE_FUNCTION_BODY
    // ASTNodeType in parser.h: AST_NODE_PROGRAM, AST_NODE_FUNCTION_DECLARATION, AST_NODE_RETURN_STATEMENT, AST_NODE_INTEGER_LITERAL
    // There isn't a specific AST_NODE_FUNCTION_BODY.
    // The base.type of FunctionBodyNode should reflect what it is, perhaps it doesn't need one if it's always part of another node.
    // Let's assume for now the type of the FunctionDeclarationNode covers the body.
    // The `base.type` for FunctionBodyNode itself is less critical if it's not treated as a standalone ASTNode in some contexts.
    // For now, let's leave it unassigned or assign it to something that makes sense if it needs a type.
    // Let's assume it does not need its own type for now, as it's a component.
    // node->base.type = ???; // Or remove if FunctionBodyNode is not an ASTNode itself directly.
    // For the previous structure, it was AST_NODE_RETURN_STATEMENT. This is no longer true.

    node->statements = NULL;
    node->statement_count = 0;
    node->capacity = 0;
    return node;
}

// Helper to add a statement to FunctionBodyNode
static void add_statement_to_body(FunctionBodyNode* body, ASTNode* statement) {
    if (!body || !statement) return;
    if (body->statement_count >= body->capacity) {
        int new_capacity = body->capacity == 0 ? 4 : body->capacity * 2;
        ASTNode** new_statements = (ASTNode**)realloc(body->statements, new_capacity * sizeof(ASTNode*));
        if (!new_statements) {
            report_error("Memory allocation failed for expanding statements array", current_token); // Use current_token as best guess
            // If realloc fails, original statements are still valid but we can't add the new one.
            // This is a critical error.
            parser_error_occurred = 1;
            // Free the statement that couldn't be added
            free_ast_node(statement);
            return;
        }
        body->statements = new_statements;
        body->capacity = new_capacity;
    }
    body->statements[body->statement_count++] = statement;
}

static FunctionDeclarationNode* create_function_declaration_node(Token name, FunctionBodyNode* body) {
    FunctionDeclarationNode* node = (FunctionDeclarationNode*)malloc(sizeof(FunctionDeclarationNode));
    if (!node) {
        report_error("Memory allocation failed for FunctionDeclarationNode", name); // use name token
        return NULL;
    }
    node->base.type = AST_NODE_FUNCTION_DECLARATION;
    node->function_name = name;
    symbol_table_init(&node->symbol_table); // Initialize symbol table
    node->body = body; // Body is parsed after symbol table is available if needed
    return node;
}

static ProgramNode* create_program_node(FunctionDeclarationNode* func_decl) {
    ProgramNode* node = (ProgramNode*)malloc(sizeof(ProgramNode));
    if (!node) {
        // No specific token for ProgramNode, this is a general allocation failure.
        // report_error needs a token. We can use the function declaration's token or a dummy.
        // This is a critical error.
        fprintf(stderr, "Critical: Memory allocation failed for ProgramNode.\n");
        parser_error_occurred = 1; // Set error flag
        return NULL;
    }
    node->base.type = AST_NODE_PROGRAM;
    node->function_declaration = func_decl;
    return node;
}

static BinaryOperationNode* create_binary_operation_node(ASTNode* left, TokenType operator_token_type, ASTNode* right) {
    BinaryOperationNode* node = (BinaryOperationNode*)malloc(sizeof(BinaryOperationNode));
    if (!node) {
        // Cannot use report_error as it needs a token. This is a critical malloc failure.
        fprintf(stderr, "Critical: Memory allocation failed for BinaryOperationNode.\n");
        parser_error_occurred = 1; // Set error flag
        // Free children if they were passed, as they won't be part of a successfully created node.
        // However, the caller might also do this. For safety, let's free them here.
        free_ast_node(left);
        free_ast_node(right);
        return NULL;
    }
    node->base.type = AST_NODE_BINARY_OPERATION;
    node->left = left;
    node->operator_token_type = operator_token_type;
    node->right = right;
    return node;
}

static ASTNode* create_assignment_statement_node(Token identifier_token, ASTNode* expression_node) {
    AssignmentStatementNode* assign_node = (AssignmentStatementNode*)malloc(sizeof(AssignmentStatementNode));
    if (!assign_node) {
        report_error("Memory allocation failed for AssignmentStatementNode", identifier_token);
        // expression_node should be freed by the caller if this fails
        return NULL;
    }
    assign_node->base.type = AST_NODE_ASSIGNMENT_STATEMENT;
    
    assign_node->variable_name = (char*)malloc(identifier_token.lexeme_length + 1);
    if (!assign_node->variable_name) {
        report_error("Memory allocation failed for assignment variable name string", identifier_token);
        free(assign_node);
        // expression_node should be freed by the caller
        return NULL;
    }
    strncpy(assign_node->variable_name, identifier_token.lexeme_start, identifier_token.lexeme_length);
    assign_node->variable_name[identifier_token.lexeme_length] = '\0';
    
    assign_node->expression = expression_node;
    return (ASTNode*)assign_node;
}

static ASTNode* create_variable_usage_node(Token identifier_token) {
    VariableUsageNode* var_node = (VariableUsageNode*)malloc(sizeof(VariableUsageNode));
    if (!var_node) {
        report_error("Memory allocation failed for VariableUsageNode", identifier_token);
        return NULL;
    }
    var_node->base.type = AST_NODE_VARIABLE_USAGE;
    
    var_node->variable_name = (char*)malloc(identifier_token.lexeme_length + 1);
    if (!var_node->variable_name) {
        report_error("Memory allocation failed for variable usage name string", identifier_token);
        free(var_node);
        return NULL;
    }
    strncpy(var_node->variable_name, identifier_token.lexeme_start, identifier_token.lexeme_length);
    var_node->variable_name[identifier_token.lexeme_length] = '\0';
    
    return (ASTNode*)var_node;
}

static ASTNode* create_if_statement_node(ASTNode* condition, FunctionBodyNode* then_block, FunctionBodyNode* else_block) {
    IfStatementNode* if_node = (IfStatementNode*)malloc(sizeof(IfStatementNode));
    if (!if_node) {
        report_error("Memory allocation failed for IfStatementNode", current_token); // Use current_token as best guess
        free_ast_node(condition);
        free_ast_node((ASTNode*)then_block);
        if (else_block) {
            free_ast_node((ASTNode*)else_block);
        }
        return NULL;
    }
    if_node->base.type = AST_NODE_IF_STATEMENT;
    if_node->condition = condition;
    if_node->then_block = then_block;
    if_node->else_block = else_block;
    return (ASTNode*)if_node;
}


// --- AST Freeing Functions ---

void free_ast_node(ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case AST_NODE_INTEGER_LITERAL:
            // No specific dynamic members in IntegerLiteralNode beyond the node itself
            free(node);
            break;
        case AST_NODE_BINARY_OPERATION:
            {
                BinaryOperationNode* bin_op_node = (BinaryOperationNode*)node;
                free_ast_node(bin_op_node->left);
                free_ast_node(bin_op_node->right);
                free(bin_op_node);
            }
            break;
        case AST_NODE_IF_STATEMENT:
            {
                IfStatementNode* if_node = (IfStatementNode*)node;
                free_ast_node(if_node->condition);
                free_ast_node((ASTNode*)if_node->then_block); // then_block is a FunctionBodyNode
                if (if_node->else_block != NULL) {
                    free_ast_node((ASTNode*)if_node->else_block); // else_block is also a FunctionBodyNode
                }
                free(if_node); // Free the IfStatementNode itself
            }
            break;
        case AST_NODE_FUNCTION_BODY: // New case for freeing FunctionBodyNode
            {
                FunctionBodyNode* body_node = (FunctionBodyNode*)node;
                for (int i = 0; i < body_node->statement_count; ++i) {
                    free_ast_node(body_node->statements[i]);
                }
                if (body_node->statements) { // Ensure statements array is not NULL before freeing
                    free(body_node->statements);
                }
                free(body_node); // Free the FunctionBodyNode itself
            }
            break;
        case AST_NODE_VARIABLE_DECLARATION:
            {
                VariableDeclarationNode* decl_node = (VariableDeclarationNode*)node;
                if (decl_node->variable_name) {
                    free(decl_node->variable_name);
                }
                // No other child AST nodes to free for VariableDeclarationNode
                free(decl_node); // Free the node itself
            }
            break;
        case AST_NODE_ASSIGNMENT_STATEMENT:
            {
                AssignmentStatementNode* assign_node = (AssignmentStatementNode*)node;
                if (assign_node->variable_name) {
                    free(assign_node->variable_name);
                }
                free_ast_node(assign_node->expression); // Free the RHS expression tree
                free(assign_node); // Free the node itself
            }
            break;
        case AST_NODE_VARIABLE_USAGE:
            {
                VariableUsageNode* var_usage_node = (VariableUsageNode*)node;
                if (var_usage_node->variable_name) {
                    free(var_usage_node->variable_name);
                }
                // No child AST nodes to free for VariableUsageNode
                free(var_usage_node); // Free the node itself
            }
            break;
        case AST_NODE_RETURN_STATEMENT:
            {
                ReturnStatementNode* ret_node = (ReturnStatementNode*)node;
                free_ast_node(ret_node->expression);
                free(ret_node);
            }
            break;
        case AST_NODE_FUNCTION_DECLARATION:
            {
                FunctionDeclarationNode* func_node = (FunctionDeclarationNode*)node;
                symbol_table_destroy(&func_node->symbol_table); // Destroy symbol table
                if (func_node->body) {
                    for (int i = 0; i < func_node->body->statement_count; ++i) {
                        free_ast_node(func_node->body->statements[i]);
                    }
                    free(func_node->body->statements); // Free the array of pointers
                    free(func_node->body);             // Free the FunctionBodyNode itself
                }
                free(func_node); // Free the node itself
            }
            break;
        case AST_NODE_PROGRAM:
             {
                ProgramNode* prog_node = (ProgramNode*)node;
                free_ast_node((ASTNode*)prog_node->function_declaration);
                free(prog_node); // Free the node itself
            }
            break;
        // AST_NODE_FUNCTION_BODY is not a type used in this switch, it's part of FunctionDeclarationNode.
        // The base 'node' is freed within each case for clarity and consistency here.
        default:
            fprintf(stderr, "Warning: Unknown AST node type %d in free_ast_node at line %d, col %d (approx).\n",
                    node->type, current_token.line, current_token.column); // current_token is a guess for location
            free(node); // Attempt to free base node anyway, if not freed by a specific case.
            break;
    }
    // Note: The pattern is to free 'node' within each case.
    // If any case does not free 'node', it should be freed here or the pattern adjusted.
    // Current implementation frees 'node' in each specific case.
}

void free_program_node(ProgramNode* program_node) {
    free_ast_node((ASTNode*)program_node);
}


// --- Parsing Logic (Recursive Descent) ---

static IntegerLiteralNode* parse_integer_literal() {
    Token int_token = eat_token(TOKEN_INTEGER_LITERAL);
    if (parser_error_occurred) return NULL;
    return create_integer_literal_node(int_token);
}

// factor : INTEGER_LITERAL | TOKEN_IDENTIFIER | TOKEN_LPAREN expression TOKEN_RPAREN
// Renamed to parse_primary_expression
static ASTNode* parse_primary_expression(SymbolTable* current_st) { 
    if (peek_token().type == TOKEN_LPAREN) {
        eat_token(TOKEN_LPAREN); 
        if (parser_error_occurred) return NULL;

        ASTNode* expr_node = parse_expression(current_st); // Call new top-level parse_expression
        if (parser_error_occurred || !expr_node) return NULL;

        eat_token(TOKEN_RPAREN);
        if (parser_error_occurred) {
            free_ast_node(expr_node);
            return NULL;
        }
        return expr_node;
    } else if (peek_token().type == TOKEN_INTEGER_LITERAL) {
        return (ASTNode*)parse_integer_literal();
    } else if (peek_token().type == TOKEN_IDENTIFIER) {
        Token identifier_token = peek_token();
        char var_name_buffer[256]; 
        if (identifier_token.lexeme_length >= 256) {
             report_error("Identifier too long", identifier_token);
             return NULL;
        }
        strncpy(var_name_buffer, identifier_token.lexeme_start, identifier_token.lexeme_length);
        var_name_buffer[identifier_token.lexeme_length] = '\0';

        if (symbol_table_lookup(current_st, var_name_buffer) == NULL) {
            char error_msg[300];
            sprintf(error_msg, "Variable '%.*s' not declared before use.", identifier_token.lexeme_length, identifier_token.lexeme_start);
            report_error(error_msg, identifier_token);
            return NULL;
        }
        eat_token(TOKEN_IDENTIFIER); 
        return create_variable_usage_node(identifier_token);
    } else {
        report_error("Expected integer literal, identifier, or '(' in expression", peek_token());
        return NULL;
    }
}

// term : factor ( (TOKEN_STAR | TOKEN_SLASH) factor )*
// Renamed to parse_multiplicative_expression
static ASTNode* parse_multiplicative_expression(SymbolTable* current_st) { 
    ASTNode* left_node = parse_primary_expression(current_st); 
    if (parser_error_occurred || !left_node) {
        return NULL; 
    }

    while (peek_token().type == TOKEN_STAR || peek_token().type == TOKEN_SLASH) {
        Token operator_token = eat_token(peek_token().type); 
        if (parser_error_occurred) { 
            free_ast_node(left_node);
            return NULL;
        }

        ASTNode* right_node = parse_primary_expression(current_st); 
        if (parser_error_occurred || !right_node) {
            free_ast_node(left_node); 
            return NULL;
        }

        left_node = (ASTNode*)create_binary_operation_node(left_node, operator_token.type, right_node);
        if (parser_error_occurred || !left_node) { 
            return NULL; 
        }
    }
    return left_node;
}

// expression : term ( (TOKEN_PLUS | TOKEN_MINUS) term )*
// Renamed to parse_additive_expression
static ASTNode* parse_additive_expression(SymbolTable* current_st) { 
    ASTNode* left_node = parse_multiplicative_expression(current_st); 
    if (parser_error_occurred || !left_node) {
        return NULL;
    }

    while (peek_token().type == TOKEN_PLUS || peek_token().type == TOKEN_MINUS) {
        Token operator_token = eat_token(peek_token().type); 
        if (parser_error_occurred) { 
            free_ast_node(left_node);
            return NULL;
        }

        ASTNode* right_node = parse_multiplicative_expression(current_st); 
        if (parser_error_occurred || !right_node) {
            free_ast_node(left_node); 
            return NULL;
        }

        left_node = (ASTNode*)create_binary_operation_node(left_node, operator_token.type, right_node);
        if (parser_error_occurred || !left_node) { 
            return NULL; 
        }
    }
    return left_node;
}

// New function for comparison expressions
// comparison_expression : additive_expression ( (TOKEN_EQ_EQ | TOKEN_NOT_EQ | ... ) additive_expression )*
static ASTNode* parse_comparison_expression(SymbolTable* current_st) {
    ASTNode* left_node = parse_additive_expression(current_st);
    if (parser_error_occurred || !left_node) {
        return NULL;
    }

    while (peek_token().type == TOKEN_EQ_EQ || peek_token().type == TOKEN_NOT_EQ ||
           peek_token().type == TOKEN_LESS || peek_token().type == TOKEN_LESS_EQ ||
           peek_token().type == TOKEN_GREATER || peek_token().type == TOKEN_GREATER_EQ) {
        
        Token operator_token = eat_token(peek_token().type);
        if (parser_error_occurred) {
            free_ast_node(left_node);
            return NULL;
        }

        ASTNode* right_node = parse_additive_expression(current_st);
        if (parser_error_occurred || !right_node) {
            free_ast_node(left_node);
            return NULL;
        }

        left_node = (ASTNode*)create_binary_operation_node(left_node, operator_token.type, right_node);
        if (parser_error_occurred || !left_node) {
            // create_binary_operation_node should handle freeing children on its own failure
            return NULL;
        }
    }
    return left_node;
}

// New top-level expression parsing function
static ASTNode* parse_expression(SymbolTable* current_st) {
    return parse_comparison_expression(current_st);
}


static ReturnStatementNode* parse_return_statement(SymbolTable* current_st) { 
    Token return_keyword_token = eat_token(TOKEN_RETURN); 
    if (parser_error_occurred) return NULL;

    ASTNode* expression = parse_expression(current_st); // Pass symbol table
    if (parser_error_occurred) return NULL; 
    if (!expression) { 
        report_error("Missing expression after 'return' keyword", return_keyword_token);
        return NULL;
    }

    eat_token(TOKEN_SEMICOLON);
    if (parser_error_occurred) {
        free_ast_node(expression);
        return NULL;
    }

    return create_return_statement_node(expression);
}

// --- AST Node Creation Helper for VariableDeclarationNode ---
static VariableDeclarationNode* create_variable_declaration_node(Token type_token, Token identifier_token) {
    VariableDeclarationNode* decl_node = (VariableDeclarationNode*)malloc(sizeof(VariableDeclarationNode));
    if (!decl_node) {
        report_error("Memory allocation failed for VariableDeclarationNode", identifier_token);
        return NULL;
    }
    decl_node->base.type = AST_NODE_VARIABLE_DECLARATION;
    decl_node->type_token = type_token; // Store the whole token for 'int'
    
    // strdup the variable name
    decl_node->variable_name = (char*)malloc(identifier_token.lexeme_length + 1);
    if (!decl_node->variable_name) {
        report_error("Memory allocation failed for variable name string", identifier_token);
        free(decl_node);
        return NULL;
    }
    strncpy(decl_node->variable_name, identifier_token.lexeme_start, identifier_token.lexeme_length);
    decl_node->variable_name[identifier_token.lexeme_length] = '\0';
    
    return decl_node;
}


// Parse a variable declaration statement: "int <identifier> ;"
static ASTNode* parse_variable_declaration_statement(SymbolTable* current_st) {
    Token type_token = eat_token(TOKEN_INT); // Expect 'int'
    if (parser_error_occurred) return NULL;

    Token identifier_token = eat_token(TOKEN_IDENTIFIER);
    if (parser_error_occurred) return NULL;

    eat_token(TOKEN_SEMICOLON);
    if (parser_error_occurred) return NULL; // Note: identifier_token.lexeme_start is not heap allocated by eat_token

    // Create AST node
    VariableDeclarationNode* decl_node = create_variable_declaration_node(type_token, identifier_token);
    if (!decl_node) { // Error in create_variable_declaration_node (malloc or strdup failed)
        // Error already reported by create_variable_declaration_node
        return NULL;
    }

    // Add to symbol table
    if (!symbol_table_add(current_st, decl_node->variable_name, type_token.type)) {
        // Error (e.g., re-declaration) already printed by symbol_table_add
        free(decl_node->variable_name); // Free the strdup'd name
        free(decl_node);                // Free the AST node
        parser_error_occurred = 1;      // Ensure error flag is set
        return NULL;
    }

    return (ASTNode*)decl_node;
}


// Parse a single statement.
static ASTNode* parse_statement(SymbolTable* current_st) {
    if (peek_token().type == TOKEN_INT) { // Variable Declaration
        return parse_variable_declaration_statement(current_st);
    } else if (peek_token().type == TOKEN_RETURN) { // Return Statement
        return (ASTNode*)parse_return_statement(current_st); // Pass current_st
    } else if (peek_token().type == TOKEN_IF) { // If Statement
        return parse_if_statement(current_st);
    } else if (peek_token().type == TOKEN_IDENTIFIER) { // Potential Assignment
        Token identifier_token = peek_token(); 

        char var_name_buffer[256]; 
        if (identifier_token.lexeme_length >= 256) {
             report_error("Identifier too long for assignment lookup", identifier_token);
             return NULL;
        }
        strncpy(var_name_buffer, identifier_token.lexeme_start, identifier_token.lexeme_length);
        var_name_buffer[identifier_token.lexeme_length] = '\0';

        if (symbol_table_lookup(current_st, var_name_buffer) == NULL) {
            char error_msg[300];
            sprintf(error_msg, "Variable '%.*s' not declared before assignment.", identifier_token.lexeme_length, identifier_token.lexeme_start);
            report_error(error_msg, identifier_token);
            return NULL;
        }
        
        eat_token(TOKEN_IDENTIFIER); 
        if (parser_error_occurred) return NULL;


        if (peek_token().type == TOKEN_EQUAL) { // Check if it's an assignment
            eat_token(TOKEN_EQUAL); 
            if (parser_error_occurred) return NULL;

            ASTNode* rhs_expr = parse_expression(current_st); 
            if (parser_error_occurred || !rhs_expr) {
                return NULL;
            }

            eat_token(TOKEN_SEMICOLON);
            if (parser_error_occurred) {
                free_ast_node(rhs_expr);
                return NULL;
            }
            return create_assignment_statement_node(identifier_token, rhs_expr);
        } else { // Not an assignment, could be a standalone expression statement in future, or error
            char error_msg[300];
            sprintf(error_msg, "Expected '=' after identifier '%.*s' for assignment, or part of an expression statement (not yet supported).", 
                    identifier_token.lexeme_length, identifier_token.lexeme_start);
            report_error(error_msg, identifier_token);
            return NULL;
        }
    }
    else {
        report_error("Expected a statement (declaration, return, if, or assignment)", peek_token());
        return NULL;
    }
}

// Function body: '{' statement* '}'
static FunctionBodyNode* parse_function_body(SymbolTable* current_st) { // Takes SymbolTable
    FunctionBodyNode* body = create_function_body_node();
    if (!body) return NULL; // Error in creation (malloc failed)

    while (peek_token().type != TOKEN_RBRACE && peek_token().type != TOKEN_EOF && !parser_error_occurred) {
        ASTNode* stmt = parse_statement(current_st); // Pass symbol table
        if (parser_error_occurred) { 
            // We just need to free the body.
            // The current free_ast_node for FunctionDeclaration will iterate statements,
            // so if any were added before error, they will be freed.
            // If stmt itself is non-NULL due to partial parse but error occurred, it needs freeing.
            // However, parse_statement is expected to return NULL on error.
            free(body->statements); // Free the array itself
            free(body); // Free the body struct
            return NULL;
        }
        if (stmt) { // If a statement was successfully parsed
            add_statement_to_body(body, stmt);
        } else {
            // If stmt is NULL and no error reported, it means parse_statement didn't find a statement
            // This case should be handled by parse_statement reporting an error.
            // If we reach here, it implies an empty part in the body, which might be okay or an error
            // depending on grammar. For now, assume parse_statement handles it.
            // If we allow empty statements (e.g. just ';'), parse_statement would return a specific node.
            // For now, assume parse_statement returns NULL if no valid statement is found and reports error.
            break; // Exit loop if no statement could be parsed.
        }
    }
    
    // For the current grammar "int main() { return <int>; }"
    // We expect exactly one statement.
    if (!parser_error_occurred && body->statement_count == 0) {
        // This can happen if the loop condition breaks due to RBRACE or EOF immediately.
        // This implies an empty function body, which is not allowed by "return <int>;"
        // The error should ideally be "expected return statement" from within the loop.
        // However, if parse_statement itself doesn't find TOKEN_RETURN and reports error,
        // parser_error_occurred would be set.
        // This check is a safeguard.
        report_error("Function body cannot be empty, expected 'return' statement.", peek_token()); // peek_token might be RBRACE here
        free(body->statements);
        free(body);
        return NULL;
    }


    return body;
}

static FunctionDeclarationNode* parse_function_declaration() {
    eat_token(TOKEN_INT); 
    if (parser_error_occurred) return NULL;

    Token function_name = eat_token(TOKEN_IDENTIFIER);
    if (parser_error_occurred) return NULL;

    // Simple check for "main"
    if (function_name.lexeme_length != 4 || strncmp(function_name.lexeme_start, "main", 4) != 0) {
        report_error("Expected function name 'main'", function_name);
        return NULL;
    }

    eat_token(TOKEN_LPAREN);
    if (parser_error_occurred) return NULL;
    eat_token(TOKEN_RPAREN);
    if (parser_error_occurred) return NULL;
    
    Token lbrace_token = eat_token(TOKEN_LBRACE);
    if (parser_error_occurred) return NULL;

    FunctionBodyNode* body = parse_function_body();
    if (parser_error_occurred) { // body should be NULL or cleaned up by parse_function_body
        return NULL;
    }
    if (!body) { // If body is NULL but no error reported (e.g. malloc failure in create_function_body_node)
        // This case should ideally be caught by parser_error_occurred flag.
        // If create_function_body_node itself reported error, flag is set.
        // If it failed silently, this is a fallback.
        if(!parser_error_occurred) report_error("Failed to parse function body.", lbrace_token); // Use LBRACE as location
        return NULL;
    }

    eat_token(TOKEN_RBRACE);
    if (parser_error_occurred) {
        // Clean up body if RBRACE is missing.
        // The body contains an array of statements and the statements themselves.
        // free_ast_node for FunctionDeclarationNode handles this, but if we error out before
        // creating the FunctionDeclarationNode, we need to free the body here.
        for (int i = 0; i < body->statement_count; ++i) {
            free_ast_node(body->statements[i]);
        }
        free(body->statements);
        free(body);
        return NULL;
    }

    return create_function_declaration_node(function_name, body);
}

static ProgramNode* parse_program() {
    // A program is a single function declaration ("main")
    FunctionDeclarationNode* func_decl = parse_function_declaration();
    if (parser_error_occurred) return NULL; // Error and cleanup handled deeper
    if (!func_decl) { // Should not happen if no error, but safeguard
        if(!parser_error_occurred) report_error("Expected function declaration 'int main() ...'", peek_token());
        return NULL;
    }

    // Expect EOF after the function declaration
    if (peek_token().type != TOKEN_EOF) {
        report_error("Expected end of input after function declaration", peek_token());
        free_ast_node((ASTNode*)func_decl); // Clean up the successfully parsed function
        return NULL;
    }

    return create_program_node(func_decl);
}


// --- Main Parsing Function ---
ProgramNode* parse(const char* source_code) {
    current_source_ptr = source_code;
    parser_error_occurred = 0;
    
    reset_lexer_state(); // Reset line and column counters in the lexer
    advance_token();     // Get the first token

    ProgramNode* program_ast = parse_program();

    if (parser_error_occurred) {
        // If program_ast is non-NULL here, it means parse_program returned something
        // despite an error being flagged. This indicates an issue in error propagation.
        // Generally, if parser_error_occurred is true, program_ast should be NULL
        // because the functions are expected to return NULL on error.
        if (program_ast) {
            free_program_node(program_ast); 
            program_ast = NULL;
        }
        // fprintf(stderr, "Parsing failed.\n"); // Error messages should have been printed by report_error
        return NULL;
    }
    
    if (!program_ast && !parser_error_occurred) {
        // This case indicates that parsing finished (EOF likely not reached or some other logic error)
        // without producing an AST and without flagging an error.
        // This is a parser bug.
        report_error("Parser completed without generating an AST and without reporting specific errors.", current_token);
        return NULL;
    }

    return program_ast;
}
