#include "codegen.h"
#include "parser.h" // For AST node types and structures
#include <stdio.h>  // For fprintf, fputs, FILE*

// --- Forward Declarations for Static Helper Functions ---
static void generate_function_declaration_assembly(FunctionDeclarationNode* func_decl_node, FILE* outfile);
static void generate_statement_assembly(ASTNode* statement_node, FILE* outfile);
static void generate_expression_assembly(ASTNode* expression_node, FILE* outfile); // Will place value in %eax

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

    // Body
    if (func_decl_node->body) {
        FunctionBodyNode* body = func_decl_node->body;
        if (body->statement_count == 0 && body->statements == NULL) {
             // This case was added during parser development for empty bodies,
             // but current grammar requires a return.
             // If parser allows empty body and we reach here, it's an issue for this codegen.
            fprintf(stderr, "Codegen Warning: Function body is empty. Emitting epilogue and ret only.\n");
        }
        for (int i = 0; i < body->statement_count; ++i) {
            if (body->statements[i]) {
                generate_statement_assembly(body->statements[i], outfile);
            } else {
                fprintf(stderr, "Codegen Error: Found NULL statement in function body.\n");
            }
        }
    } else {
        fprintf(stderr, "Codegen Error: FunctionDeclarationNode has no body.\n");
    }

    // Epilogue
    fputs("    popq %rbp\n", outfile);
    // Return
    fputs("    ret\n", outfile);
}

static void generate_statement_assembly(ASTNode* statement_node, FILE* outfile) {
    if (!statement_node) {
        fprintf(stderr, "Codegen Error: Received NULL statement_node.\n");
        return;
    }

    switch (statement_node->type) {
        case AST_NODE_RETURN_STATEMENT:
            {
                ReturnStatementNode* ret_node = (ReturnStatementNode*)statement_node;
                if (ret_node->expression) {
                    // The convention is that the result of an expression is placed in %eax (for integers)
                    generate_expression_assembly(ret_node->expression, outfile);
                    // The value is already in %eax from generate_expression_assembly,
                    // which is where it needs to be for the 'ret' instruction.
                    // No explicit 'mov' needed here if generate_expression_assembly follows this.
                } else {
                    fprintf(stderr, "Codegen Error: ReturnStatementNode has no expression. Defaulting to return 0.\n");
                    fprintf(outfile, "    movl $0, %%eax\n");
                }
            }
            break;
        default:
            fprintf(stderr, "Codegen Error: Unsupported statement type: %d\n", statement_node->type);
            // As a fallback, ensure %eax has a value (e.g., 0) if we hit an unsupported statement
            // before the function epilogue/return. This might prevent crashes but indicates an issue.
            fprintf(outfile, "    movl $0, %%eax # Fallback due to unsupported statement\n");
            break;
    }
}

static void generate_expression_assembly(ASTNode* expression_node, FILE* outfile) {
    if (!expression_node) {
        fprintf(stderr, "Codegen Error: Received NULL expression_node. Defaulting expression value to 0.\n");
        fprintf(outfile, "    movl $0, %%eax # Default for NULL expression\n");
        return;
    }

    switch (expression_node->type) {
        case AST_NODE_INTEGER_LITERAL:
            {
                IntegerLiteralNode* int_lit_node = (IntegerLiteralNode*)expression_node;
                // Move the integer literal value into %eax
                fprintf(outfile, "    movl $%d, %%eax\n", int_lit_node->value);
            }
            break;
        case AST_NODE_BINARY_OPERATION:
            {
                BinaryOperationNode* bin_op_node = (BinaryOperationNode*)expression_node;
                if (bin_op_node->operator_token_type == TOKEN_PLUS) {
                    // 1. Generate code for RHS (result will be in %eax)
                    generate_expression_assembly(bin_op_node->right, outfile);
                    // 2. Push RHS result (%eax) onto the stack
                    //    Using %rax for 64-bit stack operations, %eax is lower 32 bits of %rax
                    fprintf(outfile, "    pushq %%rax\n");
                    // 3. Generate code for LHS (result will be in %eax)
                    generate_expression_assembly(bin_op_node->left, outfile);
                    // 4. Pop RHS from stack into %rbx
                    fprintf(outfile, "    popq %%rbx\n");
                    // 5. Add %ebx (RHS) to %eax (LHS), result in %eax
                    fprintf(outfile, "    addl %%ebx, %%eax\n");
                } else {
                    fprintf(stderr, "Codegen Error: Unsupported binary operator type: %d. Defaulting expression value to 0.\n", bin_op_node->operator_token_type);
                    fprintf(outfile, "    movl $0, %%eax # Default for unsupported binary operator\n");
                }
            }
            break;
        default:
            fprintf(stderr, "Codegen Error: Unsupported expression type: %d. Defaulting expression value to 0.\n", expression_node->type);
            fprintf(outfile, "    movl $0, %%eax # Default for unsupported expression\n");
            break;
    }
}
