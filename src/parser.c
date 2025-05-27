#include "parser.h"
#include "lexer.h" // For Token, TokenType
#include "symbol_table.h" // For SymbolTable
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For strncmp, strdup
#include <stdbool.h>

// --- Global variables for parser state ---
static const char* current_source_ptr;
static Token current_token;
static int parser_error_occurred = 0;

// --- Forward declarations for recursive descent functions ---
static ProgramNode* parse_program();
static FunctionDeclarationNode* parse_function_declaration(); 
static void parse_parameter_list(FunctionDeclarationNode* func_decl_node); 
static FunctionBodyNode* parse_function_body(SymbolTable* current_st); 
static ASTNode* parse_statement(SymbolTable* current_st);             
static ASTNode* parse_variable_declaration_statement(SymbolTable* current_st);
static ReturnStatementNode* parse_return_statement(SymbolTable* current_st); 
static ASTNode* parse_expression(SymbolTable* current_st);                        
static ASTNode* parse_comparison_expression(SymbolTable* current_st);
static ASTNode* parse_additive_expression(SymbolTable* current_st);
static ASTNode* parse_multiplicative_expression(SymbolTable* current_st);
static ASTNode* parse_primary_expression(SymbolTable* current_st);
static IntegerLiteralNode* parse_integer_literal();
static ASTNode* parse_if_statement(SymbolTable* current_st); 
static FunctionBodyNode* parse_block_statement(SymbolTable* current_st); 
static ASTNode* parse_function_call_expression(Token identifier_token, SymbolTable* current_st); // New

// --- AST Node Creation Helper Prototypes (if not already above) ---
static IntegerLiteralNode* create_integer_literal_node(Token token);
static ReturnStatementNode* create_return_statement_node(ASTNode* expression, Token context_token); 
static FunctionBodyNode* create_function_body_node(Token context_token); 
static FunctionDeclarationNode* create_function_declaration_node(Token func_name_token, FunctionBodyNode* body); 
static ProgramNode* create_program_node(Token context_token); 
static BinaryOperationNode* create_binary_operation_node(ASTNode* left, TokenType operator_token_type, ASTNode* right, Token context_token); 
static ASTNode* create_assignment_statement_node(Token identifier_token, ASTNode* expression_node);
static ASTNode* create_variable_usage_node(Token identifier_token);
static ASTNode* create_if_statement_node(ASTNode* condition, FunctionBodyNode* then_block, FunctionBodyNode* else_block, Token context_token);
static VariableDeclarationNode* create_variable_declaration_node(Token type_token, Token identifier_token);
static FunctionCallNode* create_function_call_node(Token func_name_token); // New


// --- Helper Function Prototypes ---
static const char* token_type_to_string(TokenType type);
static void add_statement_to_body(FunctionBodyNode* body, ASTNode* statement);
static void add_function_to_program(ProgramNode* prog_node, FunctionDeclarationNode* func_decl);
static void add_parameter_to_function(FunctionDeclarationNode* func_decl_node, ParameterNode param);
static void add_argument_to_call_node(FunctionCallNode* call_node, ASTNode* arg_expr); // New
static char* strdup_token_lexeme(Token* t); // New


// --- Helper Function Implementations ---

// Helper to duplicate a token's lexeme
static char* strdup_token_lexeme(Token* t) {
    if (!t || !t->lexeme_start) {
        // Create a dummy token for error reporting if t or lexeme_start is NULL
        Token error_token = t ? *t : (Token){.line = current_token.line, .column = current_token.column, .type = TOKEN_UNKNOWN, .lexeme_start = "NULL", .lexeme_length = 4};
        report_error("Cannot duplicate lexeme from NULL token or lexeme_start", error_token);
        return NULL;
    }
    char* new_str = (char*)malloc(t->lexeme_length + 1);
    if (!new_str) {
        report_error("Memory allocation failed for token lexeme duplication", *t);
        return NULL;
    }
    strncpy(new_str, t->lexeme_start, t->lexeme_length);
    new_str[t->lexeme_length] = '\0';
    return new_str;
}


// Helper function to convert TokenType to a string representation
static const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_INT:             return "'int' keyword";
        case TOKEN_RETURN:          return "'return' keyword";
        case TOKEN_IF:              return "'if' keyword";
        case TOKEN_ELSE:            return "'else' keyword";
        case TOKEN_IDENTIFIER:      return "identifier";
        case TOKEN_INTEGER_LITERAL: return "integer literal";
        case TOKEN_LPAREN:          return "'('";
        case TOKEN_RPAREN:          return "')'";
        case TOKEN_LBRACE:          return "'{'";
        case TOKEN_RBRACE:          return "'}'";
        case TOKEN_SEMICOLON:       return "';'";
        case TOKEN_PLUS:            return "'+'";
        case TOKEN_MINUS:           return "'-'";
        case TOKEN_STAR:            return "'*'";
        case TOKEN_SLASH:           return "'/'";
        case TOKEN_EQUAL:           return "'=' (assignment)";
        case TOKEN_EQ_EQ:           return "'==' (equality)";
        case TOKEN_NOT_EQ:          return "'!='";
        case TOKEN_LESS:            return "'<'";
        case TOKEN_LESS_EQ:         return "'<='";
        case TOKEN_GREATER:         return "'>'";
        case TOKEN_GREATER_EQ:      return "'>='";
        case TOKEN_COMMA:           return "','";
        case TOKEN_EOF:             return "end of file";
        case TOKEN_UNKNOWN:         return "unknown token";
        default:                    return "undefined token type"; 
    }
}

static void advance_token() {
    if (current_token.type != TOKEN_EOF && !parser_error_occurred) {
        current_token = get_next_token(&current_source_ptr);
    }
}

static void report_error(const char* message, Token token) {
    fprintf(stderr, "Error: %s at line %d, column %d. ", message, token.line, token.column);
    if (token.type == TOKEN_EOF) {
        fprintf(stderr, "Unexpected end of input.\n");
    } else {
        fprintf(stderr, "Got token '%.*s' (Type: %s).\n", 
                token.lexeme_length, token.lexeme_start, 
                token_type_to_string(token.type));
    }
    parser_error_occurred = 1;
}

static Token eat_token(TokenType expected_type) {
    Token consumed_token = current_token;
    if (parser_error_occurred) {
        return current_token;
    }
    if (current_token.type == expected_type) {
        advance_token();
    } else {
        char error_msg[256];
        sprintf(error_msg, "Expected %s but found %s",
                token_type_to_string(expected_type),
                token_type_to_string(current_token.type));
        report_error(error_msg, current_token);
    }
    return consumed_token;
}

static Token peek_token() {
    return current_token;
}

// --- AST Node Creation Functions ---

static IntegerLiteralNode* create_integer_literal_node(Token token) {
    IntegerLiteralNode* node = (IntegerLiteralNode*)malloc(sizeof(IntegerLiteralNode));
    if (!node) {
        report_error("Memory allocation failed for IntegerLiteralNode", token);
        return NULL;
    }
    node->base.type = AST_NODE_INTEGER_LITERAL;
    node->token_literal = token;
    char* lexeme_str = (char*)malloc(token.lexeme_length + 1);
    if (!lexeme_str) {
        report_error("Memory allocation failed for lexeme string in IntegerLiteralNode", token);
        free(node);
        return NULL;
    }
    strncpy(lexeme_str, token.lexeme_start, token.lexeme_length);
    lexeme_str[token.lexeme_length] = '\0';
    node->value = atoi(lexeme_str);
    free(lexeme_str);
    return node;
}

static ReturnStatementNode* create_return_statement_node(ASTNode* expression, Token context_token) {
    ReturnStatementNode* node = (ReturnStatementNode*)malloc(sizeof(ReturnStatementNode));
    if (!node) {
        report_error("Memory allocation failed for ReturnStatementNode", context_token);
        free_ast_node(expression); 
        return NULL;
    }
    node->base.type = AST_NODE_RETURN_STATEMENT;
    node->expression = expression;
    return node;
}

static FunctionBodyNode* create_function_body_node(Token context_token) {
    FunctionBodyNode* node = (FunctionBodyNode*)malloc(sizeof(FunctionBodyNode));
    if (!node) {
        report_error("Memory allocation failed for FunctionBodyNode", context_token);
        return NULL;
    }
    node->base.type = AST_NODE_FUNCTION_BODY;
    node->statements = NULL;
    node->statement_count = 0;
    node->capacity = 0;
    return node;
}

static FunctionDeclarationNode* create_function_declaration_node(Token func_name_token, FunctionBodyNode* body) {
    FunctionDeclarationNode* node = (FunctionDeclarationNode*)malloc(sizeof(FunctionDeclarationNode));
    if (!node) {
        report_error("Memory allocation failed for FunctionDeclarationNode", func_name_token);
        free_ast_node((ASTNode*)body);
        return NULL;
    }
    node->base.type = AST_NODE_FUNCTION_DECLARATION;
    node->function_name_token = func_name_token;
    node->parameters = NULL;
    node->num_parameters = 0;
    node->capacity_parameters = 0;
    symbol_table_init(&node->symbol_table);
    node->body = body;
    return node;
}

static ProgramNode* create_program_node(Token context_token) {
    ProgramNode* node = (ProgramNode*)malloc(sizeof(ProgramNode));
    if (!node) {
        report_error("Memory allocation failed for ProgramNode", context_token);
        return NULL;
    }
    node->base.type = AST_NODE_PROGRAM;
    node->functions = NULL;
    node->num_functions = 0;
    node->capacity_functions = 0;
    return node;
}

static BinaryOperationNode* create_binary_operation_node(ASTNode* left, TokenType operator_token_type, ASTNode* right, Token context_token) {
    BinaryOperationNode* node = (BinaryOperationNode*)malloc(sizeof(BinaryOperationNode));
    if (!node) {
        report_error("Memory allocation failed for BinaryOperationNode", context_token);
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
        free_ast_node(expression_node);
        return NULL;
    }
    assign_node->base.type = AST_NODE_ASSIGNMENT_STATEMENT;
    assign_node->variable_name = strdup_token_lexeme(&identifier_token);
    if (!assign_node->variable_name) { // strdup_token_lexeme already reported error
        free_ast_node(expression_node);
        free(assign_node);
        return NULL;
    }
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
    var_node->variable_name = strdup_token_lexeme(&identifier_token);
     if (!var_node->variable_name) { // strdup_token_lexeme already reported error
        free(var_node);
        return NULL;
    }
    return (ASTNode*)var_node;
}

static FunctionCallNode* create_function_call_node(Token func_name_token) {
    FunctionCallNode* call_node = (FunctionCallNode*)malloc(sizeof(FunctionCallNode));
    if (!call_node) {
        report_error("Memory allocation failed for FunctionCallNode", func_name_token);
        return NULL;
    }
    call_node->base.type = AST_NODE_FUNCTION_CALL;
    call_node->function_name = strdup_token_lexeme(&func_name_token);
    if (!call_node->function_name) { // strdup_token_lexeme already reported error
        free(call_node);
        return NULL;
    }
    call_node->arguments = NULL;
    call_node->num_arguments = 0;
    call_node->capacity_arguments = 0;
    return call_node;
}


static ASTNode* create_if_statement_node(ASTNode* condition, FunctionBodyNode* then_block, FunctionBodyNode* else_block, Token context_token) {
    IfStatementNode* if_node = (IfStatementNode*)malloc(sizeof(IfStatementNode));
    if (!if_node) {
        report_error("Memory allocation failed for IfStatementNode", context_token);
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

static VariableDeclarationNode* create_variable_declaration_node(Token type_token, Token identifier_token) {
    VariableDeclarationNode* decl_node = (VariableDeclarationNode*)malloc(sizeof(VariableDeclarationNode));
    if (!decl_node) {
        report_error("Memory allocation failed for VariableDeclarationNode", identifier_token);
        return NULL;
    }
    decl_node->base.type = AST_NODE_VARIABLE_DECLARATION;
    decl_node->type_token = type_token;
    decl_node->variable_name = strdup_token_lexeme(&identifier_token);
     if (!decl_node->variable_name) { // strdup_token_lexeme already reported error
        free(decl_node);
        return NULL;
    }
    return decl_node;
}

// --- Dynamic Array Helpers ---

static void add_statement_to_body(FunctionBodyNode* body, ASTNode* statement) {
    if (!body || !statement) return;
    if (body->statement_count >= body->capacity) {
        int new_capacity = body->capacity == 0 ? 4 : body->capacity * 2;
        ASTNode** new_statements = (ASTNode**)realloc(body->statements, new_capacity * sizeof(ASTNode*));
        if (!new_statements) {
            report_error("Memory allocation failed for expanding statements array in FunctionBodyNode", current_token);
            free_ast_node(statement); 
            return;
        }
        body->statements = new_statements;
        body->capacity = new_capacity;
    }
    body->statements[body->statement_count++] = statement;
}

static void add_function_to_program(ProgramNode* prog_node, FunctionDeclarationNode* func_decl) {
    if (!prog_node || !func_decl) return;
    if (prog_node->num_functions >= prog_node->capacity_functions) {
        int new_capacity = prog_node->capacity_functions == 0 ? 4 : prog_node->capacity_functions * 2;
        FunctionDeclarationNode** new_functions =
            (FunctionDeclarationNode**)realloc(prog_node->functions, new_capacity * sizeof(FunctionDeclarationNode*));
        if (!new_functions) {
            report_error("Memory allocation failed for expanding functions array in ProgramNode", peek_token());
            free_ast_node((ASTNode*)func_decl); 
            return;
        }
        prog_node->functions = new_functions;
        prog_node->capacity_functions = new_capacity;
    }
    prog_node->functions[prog_node->num_functions++] = func_decl;
}

static void add_parameter_to_function(FunctionDeclarationNode* func_decl_node, ParameterNode param) {
    if (!func_decl_node) return;
    if (func_decl_node->num_parameters >= func_decl_node->capacity_parameters) {
        int new_capacity = func_decl_node->capacity_parameters == 0 ? 2 : func_decl_node->capacity_parameters * 2;
        ParameterNode* new_params =
            (ParameterNode*)realloc(func_decl_node->parameters, new_capacity * sizeof(ParameterNode));
        if (!new_params) {
            report_error("Memory allocation failed for expanding parameters array in FunctionDeclarationNode", peek_token());
            return;
        }
        func_decl_node->parameters = new_params;
        func_decl_node->capacity_parameters = new_capacity;
    }
    func_decl_node->parameters[func_decl_node->num_parameters++] = param;
}

static void add_argument_to_call_node(FunctionCallNode* call_node, ASTNode* arg_expr) {
    if (!call_node || !arg_expr) return;
    if (call_node->num_arguments >= call_node->capacity_arguments) {
        int new_capacity = call_node->capacity_arguments == 0 ? 4 : call_node->capacity_arguments * 2;
        ASTNode** new_args = (ASTNode**)realloc(call_node->arguments, new_capacity * sizeof(ASTNode*));
        if (!new_args) {
            report_error("Memory allocation failed for expanding arguments array in FunctionCallNode", peek_token());
            free_ast_node(arg_expr); 
            return;
        }
        call_node->arguments = new_args;
        call_node->capacity_arguments = new_capacity;
    }
    call_node->arguments[call_node->num_arguments++] = arg_expr;
}


// --- AST Freeing Functions --- 
void free_ast_node(ASTNode* node) {
    if (!node) return;
    switch (node->type) {
        case AST_NODE_INTEGER_LITERAL:
            free(node);
            break;
        case AST_NODE_BINARY_OPERATION: {
            BinaryOperationNode* bin_op_node = (BinaryOperationNode*)node;
            free_ast_node(bin_op_node->left);
            free_ast_node(bin_op_node->right);
            free(bin_op_node);
            break;
        }
        case AST_NODE_IF_STATEMENT: {
            IfStatementNode* if_node = (IfStatementNode*)node;
            free_ast_node(if_node->condition);
            free_ast_node((ASTNode*)if_node->then_block);
            if (if_node->else_block) {
                free_ast_node((ASTNode*)if_node->else_block);
            }
            free(if_node);
            break;
        }
        case AST_NODE_FUNCTION_BODY: {
            FunctionBodyNode* body_node = (FunctionBodyNode*)node;
            for (int i = 0; i < body_node->statement_count; ++i) {
                free_ast_node(body_node->statements[i]);
            }
            if (body_node->statements) {
                free(body_node->statements);
            }
            free(body_node);
            break;
        }
        case AST_NODE_VARIABLE_DECLARATION: {
            VariableDeclarationNode* decl_node = (VariableDeclarationNode*)node;
            if (decl_node->variable_name) {
                free(decl_node->variable_name);
            }
            free(decl_node);
            break;
        }
        case AST_NODE_ASSIGNMENT_STATEMENT: {
            AssignmentStatementNode* assign_node = (AssignmentStatementNode*)node;
            if (assign_node->variable_name) {
                free(assign_node->variable_name);
            }
            free_ast_node(assign_node->expression);
            free(assign_node);
            break;
        }
        case AST_NODE_VARIABLE_USAGE: {
            VariableUsageNode* var_usage_node = (VariableUsageNode*)node;
            if (var_usage_node->variable_name) {
                free(var_usage_node->variable_name);
            }
            free(var_usage_node);
            break;
        }
        case AST_NODE_RETURN_STATEMENT: {
            ReturnStatementNode* ret_node = (ReturnStatementNode*)node;
            free_ast_node(ret_node->expression);
            free(ret_node);
            break;
        }
        case AST_NODE_FUNCTION_DECLARATION: {
            FunctionDeclarationNode* func_node = (FunctionDeclarationNode*)node;
            symbol_table_destroy(&func_node->symbol_table);
            if (func_node->parameters) {
                free(func_node->parameters);
            }
            free_ast_node((ASTNode*)func_node->body);
            free(func_node);
            break;
        }
        case AST_NODE_PROGRAM: {
            ProgramNode* prog_node = (ProgramNode*)node;
            if (prog_node->functions) {
                for (int i = 0; i < prog_node->num_functions; ++i) {
                    free_ast_node((ASTNode*)prog_node->functions[i]);
                }
                free(prog_node->functions);
            }
            free(prog_node);
            break;
        }
        case AST_NODE_FUNCTION_CALL: { // New case for function calls
            FunctionCallNode* call_node = (FunctionCallNode*)node;
            if (call_node->function_name) {
                free(call_node->function_name);
            }
            if (call_node->arguments) {
                for (int i = 0; i < call_node->num_arguments; ++i) {
                    free_ast_node(call_node->arguments[i]);
                }
                free(call_node->arguments);
            }
            free(call_node);
            break;
        }
        default:
            fprintf(stderr, "Warning: Unknown AST node type %d in free_ast_node.\n", node->type);
            free(node);
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

static ASTNode* parse_primary_expression(SymbolTable* current_st) {
    if (peek_token().type == TOKEN_LPAREN) {
        eat_token(TOKEN_LPAREN);
        if (parser_error_occurred) return NULL;
        ASTNode* expr_node = parse_expression(current_st);
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
        Token identifier_token = eat_token(TOKEN_IDENTIFIER); // Consume the identifier
        if (parser_error_occurred) return NULL;

        if (peek_token().type == TOKEN_LPAREN) { // Function call
            return parse_function_call_expression(identifier_token, current_st);
        } else { // Variable usage
            // Check if variable is declared (already done by original logic, but ensure it's covered)
            char var_name_buffer[256];
             if (identifier_token.lexeme_length >= sizeof(var_name_buffer) -1 ) {
                report_error("Identifier too long", identifier_token);
                return NULL;
            }
            strncpy(var_name_buffer, identifier_token.lexeme_start, identifier_token.lexeme_length);
            var_name_buffer[identifier_token.lexeme_length] = '\0';
            if (symbol_table_lookup(current_st, var_name_buffer) == NULL) {
                 char error_msg[300];
                 sprintf(error_msg, "Variable '%.*s' not declared before use (when not a function call).", identifier_token.lexeme_length, identifier_token.lexeme_start);
                 report_error(error_msg, identifier_token);
                 return NULL;
            }
            return create_variable_usage_node(identifier_token);
        }
    } else {
        report_error("Expected integer literal, identifier, or '(' in expression", peek_token());
        return NULL;
    }
}

static ASTNode* parse_multiplicative_expression(SymbolTable* current_st) {
    ASTNode* left_node = parse_primary_expression(current_st);
    if (parser_error_occurred || !left_node) return NULL;
    while (peek_token().type == TOKEN_STAR || peek_token().type == TOKEN_SLASH) {
        Token operator_token = eat_token(peek_token().type);
        if (parser_error_occurred) { free_ast_node(left_node); return NULL; }
        ASTNode* right_node = parse_primary_expression(current_st);
        if (parser_error_occurred || !right_node) { free_ast_node(left_node); return NULL; }
        left_node = (ASTNode*)create_binary_operation_node(left_node, operator_token.type, right_node, operator_token);
        if (parser_error_occurred || !left_node) return NULL;
    }
    return left_node;
}

static ASTNode* parse_additive_expression(SymbolTable* current_st) {
    ASTNode* left_node = parse_multiplicative_expression(current_st);
    if (parser_error_occurred || !left_node) return NULL;
    while (peek_token().type == TOKEN_PLUS || peek_token().type == TOKEN_MINUS) {
        Token operator_token = eat_token(peek_token().type);
        if (parser_error_occurred) { free_ast_node(left_node); return NULL; }
        ASTNode* right_node = parse_multiplicative_expression(current_st);
        if (parser_error_occurred || !right_node) { free_ast_node(left_node); return NULL; }
        left_node = (ASTNode*)create_binary_operation_node(left_node, operator_token.type, right_node, operator_token);
        if (parser_error_occurred || !left_node) return NULL;
    }
    return left_node;
}

static ASTNode* parse_comparison_expression(SymbolTable* current_st) {
    ASTNode* left_node = parse_additive_expression(current_st);
    if (parser_error_occurred || !left_node) return NULL;
    while (peek_token().type == TOKEN_EQ_EQ || peek_token().type == TOKEN_NOT_EQ ||
           peek_token().type == TOKEN_LESS || peek_token().type == TOKEN_LESS_EQ ||
           peek_token().type == TOKEN_GREATER || peek_token().type == TOKEN_GREATER_EQ) {
        Token operator_token = eat_token(peek_token().type);
        if (parser_error_occurred) { free_ast_node(left_node); return NULL; }
        ASTNode* right_node = parse_additive_expression(current_st);
        if (parser_error_occurred || !right_node) { free_ast_node(left_node); return NULL; }
        left_node = (ASTNode*)create_binary_operation_node(left_node, operator_token.type, right_node, operator_token);
        if (parser_error_occurred || !left_node) return NULL;
    }
    return left_node;
}

static ASTNode* parse_expression(SymbolTable* current_st) {
    return parse_comparison_expression(current_st);
}

static ReturnStatementNode* parse_return_statement(SymbolTable* current_st) {
    Token return_keyword_token = eat_token(TOKEN_RETURN);
    if (parser_error_occurred) return NULL;
    ASTNode* expression = parse_expression(current_st);
    if (parser_error_occurred) return NULL; 
    if (!expression && !parser_error_occurred) { 
        report_error("Missing expression after 'return' keyword", return_keyword_token);
        return NULL;
    }
    eat_token(TOKEN_SEMICOLON);
    if (parser_error_occurred) {
        free_ast_node(expression);
        return NULL;
    }
    return create_return_statement_node(expression, return_keyword_token);
}

static ASTNode* parse_variable_declaration_statement(SymbolTable* current_st) {
    Token type_token = eat_token(TOKEN_INT);
    if (parser_error_occurred) return NULL;
    Token identifier_token = eat_token(TOKEN_IDENTIFIER);
    if (parser_error_occurred) return NULL;
    eat_token(TOKEN_SEMICOLON);
    if (parser_error_occurred) return NULL;
    
    VariableDeclarationNode* decl_node = create_variable_declaration_node(type_token, identifier_token);
    if (!decl_node) return NULL;

    int local_var_offset = current_st->current_stack_offset - 4; 

    SymbolTableStatus status = symbol_table_add(current_st, decl_node->variable_name, decl_node->type_token.type, local_var_offset);
    if (status != SYMBOL_TABLE_SUCCESS) {
        char error_msg[300];
        switch (status) {
            case SYMBOL_TABLE_ERROR_ALREADY_EXISTS:
                sprintf(error_msg, "Variable '%s' already declared in this scope.", decl_node->variable_name);
                break;
            case SYMBOL_TABLE_ERROR_TABLE_FULL:
                sprintf(error_msg, "Symbol table full. Cannot declare variable '%s'.", decl_node->variable_name);
                break;
            default: 
                sprintf(error_msg, "Failed to add variable '%s' to symbol table (status: %d).", decl_node->variable_name, status);
                break;
        }
        report_error(error_msg, identifier_token);
        
        free_ast_node((ASTNode*)decl_node); // create_variable_declaration_node mallocs name, so use free_ast_node               
        return NULL;
    }
    current_st->current_stack_offset = local_var_offset; 

    return (ASTNode*)decl_node;
}

static FunctionBodyNode* parse_block_statement(SymbolTable* current_st) {
    Token lbrace_token = eat_token(TOKEN_LBRACE);
    if (parser_error_occurred) return NULL;

    FunctionBodyNode* body_node = create_function_body_node(lbrace_token);
    if (!body_node) return NULL;

    while (peek_token().type != TOKEN_RBRACE && peek_token().type != TOKEN_EOF) {
        ASTNode* stmt = parse_statement(current_st);
        if (parser_error_occurred) { 
            free_ast_node((ASTNode*)body_node); 
            return NULL;
        }
        if (stmt) { 
             add_statement_to_body(body_node, stmt);
             if(parser_error_occurred) { 
                free_ast_node((ASTNode*)body_node);
                return NULL;
             }
        } else { 
            if (!parser_error_occurred) { 
                report_error("Expected statement or '}' in block", peek_token());
            }
            free_ast_node((ASTNode*)body_node);
            return NULL;
        }
    }

    eat_token(TOKEN_RBRACE);
    if (parser_error_occurred) { 
        free_ast_node((ASTNode*)body_node);
        return NULL;
    }
    return body_node;
}

static ASTNode* parse_if_statement(SymbolTable* current_st) {
    Token if_token = eat_token(TOKEN_IF);
    if (parser_error_occurred) return NULL;

    eat_token(TOKEN_LPAREN);
    if (parser_error_occurred) return NULL;

    ASTNode* condition_expr = parse_expression(current_st);
    if (parser_error_occurred || !condition_expr) {
        return NULL;
    }

    eat_token(TOKEN_RPAREN);
    if (parser_error_occurred) {
        free_ast_node(condition_expr);
        return NULL;
    }

    FunctionBodyNode* then_block = parse_block_statement(current_st);
    if (parser_error_occurred || !then_block) {
        free_ast_node(condition_expr);
        return NULL;
    }

    FunctionBodyNode* else_block = NULL;
    if (peek_token().type == TOKEN_ELSE) {
        eat_token(TOKEN_ELSE);
        if (parser_error_occurred) { 
            free_ast_node(condition_expr);
            free_ast_node((ASTNode*)then_block);
            return NULL;
        }
        else_block = parse_block_statement(current_st);
        if (parser_error_occurred || !else_block) {
            free_ast_node(condition_expr);
            free_ast_node((ASTNode*)then_block);
            return NULL;
        }
    }
    return create_if_statement_node(condition_expr, then_block, else_block, if_token);
}


static ASTNode* parse_statement(SymbolTable* current_st) {
    if (peek_token().type == TOKEN_INT) { 
        return parse_variable_declaration_statement(current_st);
    } else if (peek_token().type == TOKEN_RETURN) { 
        return (ASTNode*)parse_return_statement(current_st); 
    } else if (peek_token().type == TOKEN_IF) { 
        return parse_if_statement(current_st);
    } else if (peek_token().type == TOKEN_LBRACE) { 
        return (ASTNode*)parse_block_statement(current_st);
    } else if (peek_token().type == TOKEN_IDENTIFIER) { 
        // Could be an assignment or a function call statement (if func call returns void, or value ignored)
        // For now, assume expressions as statements are not directly supported unless they are assignments.
        // If it's a function call, parse_expression will handle it if it's part of RHS.
        // Here, we handle `identifier = expression;`
        Token identifier_token = peek_token(); // Don't consume yet, might be a function call in an expression context
        
        // Lookahead to see if it's a function call or assignment
        Token next_token = lexer_peek_next_token(&current_source_ptr, current_token);

        if (next_token.type == TOKEN_LPAREN) {
            // It's a function call used as a statement
            ASTNode* func_call_node = parse_function_call_expression(identifier_token, current_st);
            if(parser_error_occurred) return NULL;
            eat_token(TOKEN_SEMICOLON);
            if(parser_error_occurred) { free_ast_node(func_call_node); return NULL; }
            return func_call_node; // AST_NODE_FUNCTION_CALL
        } else if (next_token.type == TOKEN_EQUAL) {
            // It's an assignment
            eat_token(TOKEN_IDENTIFIER); // Consume identifier
             if (parser_error_occurred) return NULL;

            char var_name_buffer[256]; 
            if (identifier_token.lexeme_length >= sizeof(var_name_buffer)-1) {
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
        } else {
            report_error("Expected assignment or function call statement.", identifier_token);
            return NULL;
        }
    } else {
        report_error("Expected a statement", peek_token());
        return NULL;
    }
}

static FunctionBodyNode* parse_function_body(SymbolTable* current_st) { 
    Token lbrace_token = peek_token(); 
    FunctionBodyNode* body = create_function_body_node(lbrace_token); 
    if (!body) return NULL; 
    
    while (peek_token().type != TOKEN_RBRACE && peek_token().type != TOKEN_EOF && !parser_error_occurred) {
        ASTNode* stmt = parse_statement(current_st); 
        if (parser_error_occurred) { 
            free_ast_node((ASTNode*)body); 
            return NULL;
        }
        if (stmt) { 
            add_statement_to_body(body, stmt);
             if(parser_error_occurred) { 
                free_ast_node((ASTNode*)body);
                return NULL;
             }
        } else { 
             if (!parser_error_occurred) {
                 report_error("Expected statement or '}' in function body", peek_token());
                 free_ast_node((ASTNode*)body);
                 return NULL;
            }
            break; 
        }
    }
    return body;
}

static void parse_parameter_list(FunctionDeclarationNode* func_decl_node) {
    eat_token(TOKEN_LPAREN);
    if (parser_error_occurred) return;

    if (peek_token().type == TOKEN_RPAREN) { 
        eat_token(TOKEN_RPAREN);
        return;
    }

    Token type_token = eat_token(TOKEN_INT);
    if (parser_error_occurred) return;
    Token name_token = eat_token(TOKEN_IDENTIFIER);
    if (parser_error_occurred) return;
    
    ParameterNode param = {type_token, name_token};
    add_parameter_to_function(func_decl_node, param);
    if (parser_error_occurred) return; 

    while (peek_token().type == TOKEN_COMMA) {
        eat_token(TOKEN_COMMA);
        if (parser_error_occurred) return;

        type_token = eat_token(TOKEN_INT);
        if (parser_error_occurred) return;
        name_token = eat_token(TOKEN_IDENTIFIER);
        if (parser_error_occurred) return;

        param.type_token = type_token;
        param.name_token = name_token;
        add_parameter_to_function(func_decl_node, param);
        if (parser_error_occurred) return;
    }
    eat_token(TOKEN_RPAREN);
}


static FunctionDeclarationNode* parse_function_declaration() {
    eat_token(TOKEN_INT); 
    if (parser_error_occurred) return NULL;

    Token func_name_token = eat_token(TOKEN_IDENTIFIER); 
    if (parser_error_occurred) return NULL;

    FunctionDeclarationNode* func_node = create_function_declaration_node(func_name_token, NULL);
    if (!func_node) { 
        return NULL;
    }

    parse_parameter_list(func_node);
    if (parser_error_occurred) {
        free_ast_node((ASTNode*)func_node); 
        return NULL;
    }
    
    int current_param_offset = 8; 
    for (int i = 0; i < func_node->num_parameters; ++i) {
        ParameterNode* param_ast_node = &func_node->parameters[i];
        char param_name_str[256]; 
        if (param_ast_node->name_token.lexeme_length >= sizeof(param_name_str) -1) { 
            report_error("Parameter name too long", param_ast_node->name_token);
            free_ast_node((ASTNode*)func_node);
            return NULL;
        }
        strncpy(param_name_str, param_ast_node->name_token.lexeme_start, param_ast_node->name_token.lexeme_length);
        param_name_str[param_ast_node->name_token.lexeme_length] = '\0';
        SymbolTableStatus add_status = symbol_table_add(&func_node->symbol_table, param_name_str, param_ast_node->type_token.type, current_param_offset);
        if (add_status != SYMBOL_TABLE_SUCCESS) {
            char error_msg[300];
             switch (add_status) {
                case SYMBOL_TABLE_ERROR_ALREADY_EXISTS: sprintf(error_msg, "Duplicate parameter name '%s'.", param_name_str); break;
                default: sprintf(error_msg, "Failed to add parameter '%s' (status: %d).", param_name_str, add_status); break;
            }
            report_error(error_msg, param_ast_node->name_token);
            free_ast_node((ASTNode*)func_node);
            return NULL;
        }
        current_param_offset += 4; 
    }
    
    Token lbrace_token = eat_token(TOKEN_LBRACE);
    if (parser_error_occurred) {
        free_ast_node((ASTNode*)func_node);
        return NULL;
    }

    FunctionBodyNode* body = parse_function_body(&func_node->symbol_table);
    if (parser_error_occurred) {
        free_ast_node((ASTNode*)func_node); 
        return NULL;
    }
    if (!body && !parser_error_occurred) { 
        report_error("Failed to parse function body.", lbrace_token);
        free_ast_node((ASTNode*)func_node);
        return NULL;
    }
    func_node->body = body; 

    eat_token(TOKEN_RBRACE);
    if (parser_error_occurred) {
        free_ast_node((ASTNode*)func_node);
        return NULL;
    }
    return func_node;
}

static ProgramNode* parse_program() {
    Token start_token = peek_token();
    ProgramNode* prog_node = create_program_node(start_token); 
    if (!prog_node) { 
        return NULL;
    }

    while (peek_token().type != TOKEN_EOF && !parser_error_occurred) {
        if (peek_token().type == TOKEN_INT) { 
            FunctionDeclarationNode* func_decl = parse_function_declaration();
            if (parser_error_occurred) { 
                free_ast_node((ASTNode*)prog_node);
                return NULL;
            }
            if (func_decl) { 
                add_function_to_program(prog_node, func_decl);
                if (parser_error_occurred) { 
                    free_ast_node((ASTNode*)prog_node); 
                    return NULL;
                }
            } else { 
                 report_error("Expected function declaration but parsing failed.", peek_token());
                 free_ast_node((ASTNode*)prog_node);
                 return NULL;
            }
        } else {
            report_error("Expected 'int' keyword to start function declaration or EOF.", peek_token());
            free_ast_node((ASTNode*)prog_node);
            return NULL;
        }
    }

    if (parser_error_occurred) { 
        free_ast_node((ASTNode*)prog_node);
        return NULL;
    }

    if (prog_node->num_functions == 0) { 
        report_error("Program must contain at least one function.", current_token); 
        free_ast_node((ASTNode*)prog_node);
        return NULL;
    }
    
    return prog_node;
}

// Parse a function call expression: IDENTIFIER '(' (expression (',' expression)*)? ')'
static ASTNode* parse_function_call_expression(Token func_name_token, SymbolTable* current_st) {
    eat_token(TOKEN_LPAREN); // LPAREN was already peeked by parse_primary_expression
    if (parser_error_occurred) return NULL;

    FunctionCallNode* call_node = create_function_call_node(func_name_token);
    if (!call_node) return NULL;

    if (peek_token().type != TOKEN_RPAREN) { 
        while (true) {
            ASTNode* arg_expr = parse_expression(current_st);
            if (parser_error_occurred || !arg_expr) {
                free_ast_node((ASTNode*)call_node); // Frees name, args array, and node itself
                return NULL;
            }
            add_argument_to_call_node(call_node, arg_expr);
            if (parser_error_occurred) { // Error in add_argument (realloc fail)
                // arg_expr was freed by add_argument_to_call_node
                free_ast_node((ASTNode*)call_node); // Frees name, other args, args array, and node
                return NULL;
            }

            if (peek_token().type == TOKEN_RPAREN) {
                break; 
            } else if (peek_token().type == TOKEN_COMMA) {
                eat_token(TOKEN_COMMA); 
                if (parser_error_occurred) { 
                     free_ast_node((ASTNode*)call_node);
                     return NULL;
                }
                 if (peek_token().type == TOKEN_RPAREN) { // Trailing comma before )
                    report_error("Unexpected ')' after comma in argument list.", peek_token());
                    free_ast_node((ASTNode*)call_node);
                    return NULL;
                }
            } else {
                report_error("Expected ',' or ')' in function call argument list", peek_token());
                free_ast_node((ASTNode*)call_node);
                return NULL;
            }
        }
    }

    eat_token(TOKEN_RPAREN);
    if (parser_error_occurred) {
        free_ast_node((ASTNode*)call_node);
        return NULL;
    }

    return (ASTNode*)call_node;
}


// --- Main Parsing Function ---
ProgramNode* parse(const char* source_code) {
    current_source_ptr = source_code;
    parser_error_occurred = 0;
    
    reset_lexer_state(); 
    advance_token();     

    ProgramNode* program_ast = parse_program();

    if (parser_error_occurred) {
        if (program_ast) {
            free_program_node(program_ast); 
            program_ast = NULL;
        }
        return NULL;
    }
    
    if (!program_ast && !parser_error_occurred) {
        report_error("Parser completed without generating an AST and without reporting specific errors.", current_token);
        return NULL;
    }
    return program_ast;
}

[end of src/parser.c]
