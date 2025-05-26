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
static FunctionBodyNode* parse_function_body();
static ASTNode* parse_statement(); // Changed from parse_return_statement
static ReturnStatementNode* parse_return_statement(); // Still needed, called by parse_statement
static ASTNode* parse_expression();
static ASTNode* parse_term();    // New for operator precedence
static ASTNode* parse_factor();  // New for operator precedence
static IntegerLiteralNode* parse_integer_literal();

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
    node->base.type = AST_NODE_FUNCTION_DECLARATION; // This seems incorrect, FunctionBody is not a declaration.
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
    node->body = body;
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
                if (func_node->body) {
                    for (int i = 0; i < func_node->body->statement_count; ++i) {
                        free_ast_node(func_node->body->statements[i]);
                    }
                    free(func_node->body->statements); // Free the array of pointers
                    free(func_node->body);             // Free the FunctionBodyNode itself
                }
                free(func_node);
            }
            break;
        case AST_NODE_PROGRAM:
             {
                ProgramNode* prog_node = (ProgramNode*)node;
                free_ast_node((ASTNode*)prog_node->function_declaration);
                free(prog_node);
            }
            break;
        // AST_NODE_FUNCTION_BODY is not a type used in this switch, it's part of FunctionDeclarationNode
        default:
            fprintf(stderr, "Warning: Unknown AST node type %d in free_ast_node at line %d, col %d (approx).\n",
                    node->type, current_token.line, current_token.column); // current_token is a guess for location
            free(node); // Attempt to free base node anyway
            break;
    }
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

static ASTNode* parse_expression() {
    ASTNode* left_node = (ASTNode*)parse_integer_literal();
    if (parser_error_occurred || !left_node) {
        // Error already reported by parse_integer_literal or it returned NULL.
        return NULL;
    }

    // Check for a potential binary operator (currently only '+')
    if (peek_token().type == TOKEN_PLUS) {
        Token operator_token = eat_token(TOKEN_PLUS); // Consume the '+' token
        if (parser_error_occurred) { // Check if eat_token failed
            free_ast_node(left_node); // Clean up the left node
            return NULL;
        }

        ASTNode* right_node = (ASTNode*)parse_integer_literal();
        if (parser_error_occurred || !right_node) {
            // Error reported by parse_integer_literal or it returned NULL for the right operand.
            free_ast_node(left_node); // Clean up the successfully parsed left node
            // right_node is either NULL or its cleanup is handled by parse_integer_literal on error
            return NULL;
        }

        // Create and return the binary operation node
        return (ASTNode*)create_binary_operation_node(left_node, operator_token.type, right_node);
    }

    // If no operator follows, it's just the initially parsed left_node (integer literal)
    return left_node; // This is the fully parsed expression
}

// factor : INTEGER_LITERAL | TOKEN_LPAREN expression TOKEN_RPAREN
static ASTNode* parse_factor() {
    if (peek_token().type == TOKEN_LPAREN) {
        eat_token(TOKEN_LPAREN); // Consume '('
        if (parser_error_occurred) {
            return NULL; // Error reported by eat_token
        }

        ASTNode* expr_node = parse_expression(); // Parse the sub-expression
        if (parser_error_occurred || !expr_node) {
            // Error reported by parse_expression or it returned NULL.
            // No need to free expr_node here as it should be NULL or handled by parse_expression.
            return NULL;
        }

        // Expect and consume ')'
        Token rparen_token = eat_token(TOKEN_RPAREN);
        if (parser_error_occurred) { // eat_token failed (missing ')')
            free_ast_node(expr_node); // Clean up the successfully parsed sub-expression
            return NULL;
        }
        // If eat_token did not set parser_error_occurred but returned a non-RPAREN token (should not happen with current eat_token)
        // this would be an issue. Assuming eat_token sets the flag on mismatch.
        
        return expr_node;
    } else if (peek_token().type == TOKEN_INTEGER_LITERAL) {
        ASTNode* node = (ASTNode*)parse_integer_literal();
        if (parser_error_occurred) { // If parse_integer_literal failed
            return NULL;
        }
        // No need for the explicit !node check here if parse_integer_literal always reports on failure
        // and sets parser_error_occurred or returns NULL consistently.
        // The original check:
        // if (!node) { 
        //     if (!parser_error_occurred) { report_error("Expected an integer literal", peek_token()); }
        //     return NULL;
        // }
        return node;
    } else {
        report_error("Expected an integer literal or '(' for an expression", peek_token());
        return NULL;
    }
}

// term : factor ( (TOKEN_STAR | TOKEN_SLASH) factor )*
static ASTNode* parse_term() {
    ASTNode* left_node = parse_factor();
    if (parser_error_occurred || !left_node) {
        return NULL; // Error already reported by parse_factor or its children
    }

    while (peek_token().type == TOKEN_STAR || peek_token().type == TOKEN_SLASH) {
        Token operator_token = eat_token(peek_token().type); // Consume TOKEN_STAR or TOKEN_SLASH
        if (parser_error_occurred) { // eat_token failed
            free_ast_node(left_node);
            return NULL;
        }

        ASTNode* right_node = parse_factor();
        if (parser_error_occurred || !right_node) {
            // Error reported by parse_factor or its children for the right operand.
            free_ast_node(left_node); // Clean up the successfully parsed left node.
            return NULL;
        }

        left_node = (ASTNode*)create_binary_operation_node(left_node, operator_token.type, right_node);
        if (parser_error_occurred || !left_node) { // create_binary_operation_node failed
            // create_binary_operation_node should free its children on failure,
            // so left_node and right_node (original values) would have been freed.
            // If left_node became NULL due to this, the loop condition will handle it or next check.
            return NULL; // Propagate error
        }
    }
    return left_node;
}

// expression : term ( (TOKEN_PLUS | TOKEN_MINUS) term )*
// (Modified from the previous simple version)
static ASTNode* parse_expression() {
    ASTNode* left_node = parse_term();
    if (parser_error_occurred || !left_node) {
        // Error already reported by parse_term or its children.
        return NULL;
    }

    while (peek_token().type == TOKEN_PLUS || peek_token().type == TOKEN_MINUS) {
        Token operator_token = eat_token(peek_token().type); // Consume TOKEN_PLUS or TOKEN_MINUS
        if (parser_error_occurred) { // eat_token failed
            free_ast_node(left_node);
            return NULL;
        }

        ASTNode* right_node = parse_term();
        if (parser_error_occurred || !right_node) {
            // Error reported by parse_term or its children for the right operand.
            free_ast_node(left_node); // Clean up the successfully parsed left node.
            return NULL;
        }

        left_node = (ASTNode*)create_binary_operation_node(left_node, operator_token.type, right_node);
        if (parser_error_occurred || !left_node) { // create_binary_operation_node failed
            // create_binary_operation_node should free its children on failure.
            return NULL; // Propagate error
        }
    }
    return left_node;
}


static ReturnStatementNode* parse_return_statement() {
    Token return_keyword_token = eat_token(TOKEN_RETURN); // Keep for location if needed
    if (parser_error_occurred) return NULL;

    ASTNode* expression = parse_expression();
    if (parser_error_occurred) return NULL; // Error already reported by parse_expression or its children
    if (!expression) { // Should not happen if no error occurred, but as a safeguard
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

// Parse a single statement. For now, only return statements.
static ASTNode* parse_statement() {
    if (peek_token().type == TOKEN_RETURN) {
        return (ASTNode*)parse_return_statement();
    }
    // Future: add other statement types like variable declarations, assignments, if-statements, etc.
    // else if (peek_token().type == TOKEN_INT) { return parse_variable_declaration(); }
    else {
        report_error("Expected a statement (e.g., 'return')", peek_token());
        return NULL;
    }
}

// Function body: '{' statement* '}'
// For now, it expects exactly one 'return' statement.
// The structure is set up for multiple statements.
static FunctionBodyNode* parse_function_body() {
    FunctionBodyNode* body = create_function_body_node();
    if (!body) return NULL; // Error in creation (malloc failed)

    // Loop to parse multiple statements (though current grammar only allows one 'return')
    // This loop will effectively run once for "return <expr>;"
    // and then expect '}'
    while (peek_token().type != TOKEN_RBRACE && peek_token().type != TOKEN_EOF && !parser_error_occurred) {
        ASTNode* stmt = parse_statement();
        if (parser_error_occurred) { // If parse_statement failed
            // stmt might be NULL or partially constructed.
            // parse_statement should handle its own cleanup on error.
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
