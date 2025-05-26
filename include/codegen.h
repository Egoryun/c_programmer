#ifndef CODEGEN_H
#define CODEGEN_H

#include "parser.h" // For ProgramNode, SymbolTable, and other AST node definitions
#include <stdio.h>  // For FILE*

// Forward declare SymbolTable if parser.h doesn't expose it directly in a way codegen.h can use
// (Assuming parser.h includes symbol_table.h or defines SymbolTable sufficiently)

// Main code generation function
void generate_assembly(ProgramNode* ast, FILE* outfile);

// Note: The helper functions below are typically static to codegen.c.
// If they were to be part of a public API (e.g., for unit testing individual parts),
// their prototypes would go here. For now, they are static.
//
// static void generate_function_declaration_assembly(FunctionDeclarationNode* func_decl_node, FILE* outfile);
// static void generate_statement_assembly(ASTNode* statement_node, SymbolTable* st, FILE* outfile);
// static void generate_expression_assembly(ASTNode* expression_node, SymbolTable* st, FILE* outfile);


#endif // CODEGEN_H
