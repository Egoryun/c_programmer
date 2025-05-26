#ifndef CODEGEN_H
#define CODEGEN_H

#include "parser.h" // For ProgramNode and other AST node definitions
#include <stdio.h>  // For FILE*

// Function prototype for the main code generation function
// Takes the root of the AST (ProgramNode) and an output file stream.
void generate_assembly(ProgramNode* ast, FILE* outfile);

#endif // CODEGEN_H
