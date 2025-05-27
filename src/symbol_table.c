#include "symbol_table.h"
#include <string.h>   // For strdup, strcmp
#include <stdio.h>    // For fprintf (used only if strdup fails and no other report mechanism)
#include <stdlib.h>   // For free
#include <stdbool.h>  // For bool, though SymbolTableStatus is used now

void symbol_table_init(SymbolTable* st) {
    if (!st) return;
    st->count = 0;
    // current_stack_offset is for local variables, managed by the parser when adding locals.
    // It starts at 0, and the parser will decrement it for locals (e.g., -4, -8).
    st->current_stack_offset = 0; 
}

SymbolTableStatus symbol_table_add(SymbolTable* st, const char* name, TokenType type, int stack_offset) {
    if (!st || !name) {
        // This case should ideally not be reached if callers are valid.
        // If it is, it's an internal error. Not a typical symbol table status.
        // For robustness, let's consider it a form of malloc failure if args are bad.
        return SYMBOL_TABLE_ERROR_MALLOC_FAILED; 
    }

    // Check if symbol already exists (basic check, full scope handling is more complex)
    // For current single-scope-per-function, this is sufficient.
    if (symbol_table_lookup(st, name) != NULL) {
        return SYMBOL_TABLE_ERROR_ALREADY_EXISTS;
    }

    // Check for table overflow
    if (st->count >= MAX_SYMBOLS) {
        return SYMBOL_TABLE_ERROR_TABLE_FULL;
    }

    char* name_copy = strdup(name);
    if (!name_copy) {
        // perror("strdup failed in symbol_table_add"); // Optional: log to stderr
        return SYMBOL_TABLE_ERROR_MALLOC_FAILED;
    }
    
    st->symbols[st->count].name = name_copy;
    st->symbols[st->count].type = type; 
    st->symbols[st->count].stack_offset = stack_offset; // Use provided offset directly
    st->symbols[st->count].scope_level = 1; // Default to global/main scope for now

    st->count++;
    return SYMBOL_TABLE_SUCCESS;
}

Symbol* symbol_table_lookup(SymbolTable* st, const char* name) {
    if (!st || !name) return NULL;

    // Iterate backwards to find the most recent declaration (handles shadowing in future if scopes are nested)
    for (int i = st->count - 1; i >= 0; i--) {
        if (st->symbols[i].name && strcmp(st->symbols[i].name, name) == 0) {
            return &st->symbols[i];
        }
    }
    return NULL; // Not found
}

void symbol_table_destroy(SymbolTable* st) {
    if (!st) return;

    for (int i = 0; i < st->count; i++) {
        if (st->symbols[i].name) {
            free(st->symbols[i].name);
            st->symbols[i].name = NULL; 
        }
    }
    st->count = 0;
    st->current_stack_offset = 0; 
}
