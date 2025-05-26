#include "codegen.h"
#include "parser.h" // For AST node types and structures
#include <stdio.h>  // For fprintf, fputs, FILE*

// --- Global Static Label Counter ---
static int label_count = 0;

// Helper to generate unique label names
static void generate_new_label_name(char* buffer, size_t buffer_size, const char* prefix) {
    snprintf(buffer, buffer_size, "_%s%d", prefix, label_count++);
}

// --- Forward Declarations for Static Helper Functions ---
static void generate_function_declaration_assembly(FunctionDeclarationNode* func_decl_node, FILE* outfile);
static void generate_statement_assembly(ASTNode* statement_node, SymbolTable* st, FILE* outfile);
static void generate_expression_assembly(ASTNode* expression_node, SymbolTable* st, FILE* outfile);

// --- Main Code Generation Function ---
void generate_assembly(ProgramNode* program_ast, FILE* outfile) {
    if (!program_ast) {
        fprintf(stderr, "Codegen Error: Received NULL ProgramNode.\n");
        return;
    }
    if (!outfile) {
        fprintf(stderr, "Codegen Error: Received NULL output file.\n");
        return;
    }

    // A program consists of a single function declaration (main)
    if (program_ast->function_declaration) {
        generate_function_declaration_assembly(program_ast->function_declaration, outfile);
    } else {
        fprintf(stderr, "Codegen Error: ProgramNode has no function declaration.\n");
    }
}

// --- Static Helper Function Implementations ---

static void generate_function_declaration_assembly(FunctionDeclarationNode* func_decl_node, FILE* outfile) {
    if (!func_decl_node) {
        fprintf(stderr, "Codegen Error: Received NULL FunctionDeclarationNode.\n");
        return;
    }

    // Emit global directive for the function name
    // Assuming function_name token holds "main"
    fprintf(outfile, ".globl %.*s\n", func_decl_node->function_name.lexeme_length, func_decl_node->function_name.lexeme_start);
    // Emit label for the function
    fprintf(outfile, "%.*s:\n", func_decl_node->function_name.lexeme_length, func_decl_node->function_name.lexeme_start);

    // Prologue
    fputs("    pushq %rbp\n", outfile);
    fputs("    movq %rsp, %rbp\n", outfile);

    // Calculate stack space for local variables
    int total_var_size = -(func_decl_node->symbol_table.current_stack_offset); 
    if (total_var_size < 0) total_var_size = 0; 

    if (total_var_size > 0) {
        fprintf(outfile, "    subq $%d, %%rsp\n", total_var_size);
    }

    // Body
    if (func_decl_node->body) {
        FunctionBodyNode* body = func_decl_node->body;
        for (int i = 0; i < body->statement_count; ++i) {
            if (body->statements[i]) {
                generate_statement_assembly(body->statements[i], &func_decl_node->symbol_table, outfile);
            } else {
                fprintf(stderr, "Codegen Error: Found NULL statement in function body.\n");
            }
        }
    } else {
        fprintf(stderr, "Codegen Error: FunctionDeclarationNode has no body.\n");
    }

    // Epilogue
    fputs("    movq %rbp, %rsp\n", outfile); // Deallocate locals
    fputs("    popq %rbp\n", outfile);
    fputs("    ret\n", outfile);
}

static void generate_statement_assembly(ASTNode* statement_node, SymbolTable* st, FILE* outfile) { // Modified
    if (!statement_node) {
        fprintf(stderr, "Codegen Error: Received NULL statement_node.\n");
        return;
    }

    switch (statement_node->type) {
        case AST_NODE_VARIABLE_DECLARATION:
            // No executable code generated for declaration, space allocated in prologue.
            break;
        case AST_NODE_ASSIGNMENT_STATEMENT:
            {
                AssignmentStatementNode* assign_node = (AssignmentStatementNode*)statement_node;
                generate_expression_assembly(assign_node->expression, st, outfile); // RHS value in %eax
                
                Symbol* symbol = symbol_table_lookup(st, assign_node->variable_name);
                if (symbol) {
                    fprintf(outfile, "    movl %%eax, %d(%%rbp)\n", symbol->stack_offset);
                } else {
                    fprintf(stderr, "Codegen Error: Variable '%s' not found in symbol table for assignment (should be caught by parser).\n", assign_node->variable_name);
                }
            }
            break;
        case AST_NODE_IF_STATEMENT:
            {
                IfStatementNode* if_node = (IfStatementNode*)statement_node;
                char else_label[32];
                char end_if_label[32];

                // Generate code for the condition
                generate_expression_assembly(if_node->condition, st, outfile);
                fprintf(outfile, "    cmpl $0, %%eax\n"); // Compare result with 0 (false)

                if (if_node->else_block != NULL) {
                    generate_new_label_name(else_label, sizeof(else_label), "Lelse");
                    generate_new_label_name(end_if_label, sizeof(end_if_label), "Lendif");
                    fprintf(outfile, "    je %s\n", else_label); // If false, jump to else_label
                } else {
                    // No else block, else_label is effectively end_if_label
                    generate_new_label_name(end_if_label, sizeof(end_if_label), "Lendif");
                    strcpy(else_label, end_if_label); // For clarity or if needed, though not strictly used if no else block
                    fprintf(outfile, "    je %s\n", end_if_label); // If false, jump to end_if_label
                }

                // Then block
                // fprintf(outfile, "    # Then block for if\n");
                if (if_node->then_block) { // Should always exist for a valid if
                    for (int i = 0; i < if_node->then_block->statement_count; ++i) {
                        generate_statement_assembly(if_node->then_block->statements[i], st, outfile);
                    }
                }

                if (if_node->else_block != NULL) {
                    fprintf(outfile, "    jmp %s\n", end_if_label); // Jump to end_if after then_block
                    fprintf(outfile, "%s:\n", else_label);      // Else label
                    // fprintf(outfile, "    # Else block for if\n");
                    for (int i = 0; i < if_node->else_block->statement_count; ++i) {
                        generate_statement_assembly(if_node->else_block->statements[i], st, outfile);
                    }
                }
                // Emit end_if_label. If there was no else block, this is where 'je' jumps.
                // If there was an else block, this is where the 'jmp' from then_block jumps.
                fprintf(outfile, "%s:\n", end_if_label);
            }
            break;
        case AST_NODE_RETURN_STATEMENT:
            {
                ReturnStatementNode* ret_node = (ReturnStatementNode*)statement_node;
                if (ret_node->expression) {
                    generate_expression_assembly(ret_node->expression, st, outfile); // Pass symbol table
                } else {
                    fprintf(stderr, "Codegen Error: ReturnStatementNode has no expression. Defaulting to return 0.\n");
                    fprintf(outfile, "    movl $0, %%eax\n");
                }
            }
            break;
        default:
            fprintf(stderr, "Codegen Error: Unsupported statement type: %d\n", statement_node->type);
            fprintf(outfile, "    movl $0, %%eax # Fallback due to unsupported statement\n");
            break;
    }
}

static void generate_expression_assembly(ASTNode* expression_node, SymbolTable* st, FILE* outfile) { // Modified
    if (!expression_node) {
        fprintf(stderr, "Codegen Error: Received NULL expression_node. Defaulting expression value to 0.\n");
        fprintf(outfile, "    movl $0, %%eax # Default for NULL expression\n");
        return;
    }

    switch (expression_node->type) {
        case AST_NODE_INTEGER_LITERAL:
            {
                IntegerLiteralNode* int_lit_node = (IntegerLiteralNode*)expression_node;
                fprintf(outfile, "    movl $%d, %%eax\n", int_lit_node->value);
            }
            break;
        case AST_NODE_VARIABLE_USAGE:
            {
                VariableUsageNode* var_node = (VariableUsageNode*)expression_node;
                Symbol* symbol = symbol_table_lookup(st, var_node->variable_name);
                if (symbol) {
                    fprintf(outfile, "    movl %d(%%rbp), %%eax\n", symbol->stack_offset);
                } else {
                    fprintf(stderr, "Codegen Error: Variable '%s' not found in symbol table for usage (should be caught by parser).\n", var_node->variable_name);
                    fprintf(outfile, "    movl $0, %%eax # Default for undeclared variable\n");
                }
            }
            break;
        case AST_NODE_BINARY_OPERATION:
            {
                BinaryOperationNode* bin_op_node = (BinaryOperationNode*)expression_node;
                
                // 1. Generate code for RHS (result will be in %eax)
                generate_expression_assembly(bin_op_node->right, st, outfile); // Pass st
                // 2. Push RHS result (%eax) onto the stack
                fprintf(outfile, "    pushq %%rax\n");
                // 3. Generate code for LHS (result will be in %eax)
                generate_expression_assembly(bin_op_node->left, st, outfile); // Pass st
                // 4. Pop RHS from stack into %rbx
                fprintf(outfile, "    popq %%rbx\n");
                // 5. Perform operation based on token type
                switch (bin_op_node->operator_token_type) {
                    case TOKEN_PLUS:
                        fprintf(outfile, "    addl %%ebx, %%eax\n");
                        break;
                    case TOKEN_MINUS:
                        fprintf(outfile, "    subl %%ebx, %%eax\n");
                        break;
                    case TOKEN_STAR:
                        fprintf(outfile, "    imull %%ebx, %%eax\n");
                        break;
                    case TOKEN_SLASH:
                        fprintf(outfile, "    cdq\n");
                        fprintf(outfile, "    idivl %%ebx\n");
                        break;
                        // Comparison Operators
                        case TOKEN_EQ_EQ:
                            fprintf(outfile, "    cmpl %%ebx, %%eax\n");
                            fprintf(outfile, "    sete %%al\n");
                            fprintf(outfile, "    movzbl %%al, %%eax\n");
                            break;
                        case TOKEN_NOT_EQ:
                            fprintf(outfile, "    cmpl %%ebx, %%eax\n");
                            fprintf(outfile, "    setne %%al\n");
                            fprintf(outfile, "    movzbl %%al, %%eax\n");
                            break;
                        case TOKEN_LESS:
                            fprintf(outfile, "    cmpl %%ebx, %%eax\n");
                            fprintf(outfile, "    setl %%al\n");
                            fprintf(outfile, "    movzbl %%al, %%eax\n");
                            break;
                        case TOKEN_LESS_EQ:
                            fprintf(outfile, "    cmpl %%ebx, %%eax\n");
                            fprintf(outfile, "    setle %%al\n");
                            fprintf(outfile, "    movzbl %%al, %%eax\n");
                            break;
                        case TOKEN_GREATER:
                            fprintf(outfile, "    cmpl %%ebx, %%eax\n");
                            fprintf(outfile, "    setg %%al\n");
                            fprintf(outfile, "    movzbl %%al, %%eax\n");
                            break;
                        case TOKEN_GREATER_EQ:
                            fprintf(outfile, "    cmpl %%ebx, %%eax\n");
                            fprintf(outfile, "    setge %%al\n");
                            fprintf(outfile, "    movzbl %%al, %%eax\n");
                            break;
                    default:
                        fprintf(stderr, "Codegen Error: Unsupported binary operator type: %d. Defaulting expression value to 0.\n", bin_op_node->operator_token_type);
                        fprintf(outfile, "    movl $0, %%eax # Default for unsupported binary operator\n");
                        break;
                }
            }
            break;
        default:
            fprintf(stderr, "Codegen Error: Unsupported expression type: %d. Defaulting expression value to 0.\n", expression_node->type);
            fprintf(outfile, "    movl $0, %%eax # Default for unsupported expression\n");
            break;
    }
}
