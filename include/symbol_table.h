#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "lexer.h"   // For TokenType
#include <stdbool.h> // For bool type

#define MAX_SYMBOLS 50

// Symbol Structure
typedef struct {
    char* name;         // Variable name (will be strdup'd)
    TokenType type;     // e.g., TOKEN_INT
    int stack_offset;   // e.g., -4, -8 relative to RBP
    int scope_level;    // For now, can default to 1 (main's scope)
} Symbol;

// SymbolTable Structure
typedef struct {
    Symbol symbols[MAX_SYMBOLS];
    int count;
    int current_stack_offset; // To help assign next available offset
} SymbolTable;

// Function Prototypes
void symbol_table_init(SymbolTable* st);
bool symbol_table_add(SymbolTable* st, const char* name, TokenType type);
Symbol* symbol_table_lookup(SymbolTable* st, const char* name);
void symbol_table_destroy(SymbolTable* st);

#endif // SYMBOL_TABLE_H
