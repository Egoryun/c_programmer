#include "symbol_table.h"
#include <string.h>   // For strdup, strcmp
#include <stdio.h>    // For fprintf, perror (if needed for strdup)
#include <stdlib.h>   // For free (if strdup fails, though not explicitly handled beyond check)
#include <stdbool.h>  // Already included in .h but good practice if used directly

void symbol_table_init(SymbolTable* st) {
    if (!st) return;
    st->count = 0;
    st->current_stack_offset = 0; // First variable will be at -4, then -8, etc.
    // No need to initialize individual symbol names to NULL,
    // as they are only accessed up to st->count, and strdup handles new allocations.
}

bool symbol_table_add(SymbolTable* st, const char* name, TokenType type) {
    if (!st || !name) return SYMBOL_TABLE_ERROR_MALLOC_FAILED; // Or some other error for invalid args

    // Check if symbol already exists
    if (symbol_table_lookup(st, name) != NULL) {
        return SYMBOL_TABLE_ERROR_ALREADY_EXISTS;
    }

    // Check for table overflow
    if (st->count >= MAX_SYMBOLS) {
        return SYMBOL_TABLE_ERROR_TABLE_FULL;
    }

    // Calculate new stack offset (assuming 4-byte integers for now)
    int new_offset = st->current_stack_offset - 4;

    // Store the symbol
    char* name_copy = strdup(name);
    if (!name_copy) {
        // perror("Error: strdup failed to copy variable name"); // Keep perror for system error context if desired, but don't fprintf
        return SYMBOL_TABLE_ERROR_MALLOC_FAILED;
    }
    
    st->current_stack_offset = new_offset; // Commit offset change only after strdup success

    st->symbols[st->count].name = name_copy;
    st->symbols[st->count].type = type; // For now, only TOKEN_INT for variables
    st->symbols[st->count].stack_offset = st->current_stack_offset;
    st->symbols[st->count].scope_level = 1; // Default to global/main scope for now

    st->count++;
    return SYMBOL_TABLE_SUCCESS;
}

Symbol* symbol_table_lookup(SymbolTable* st, const char* name) {
    if (!st || !name) return NULL;

    // Iterate backwards to find the most recent declaration (handles shadowing in future)
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
            st->symbols[i].name = NULL; // Good practice
        }
    }
    st->count = 0;
    st->current_stack_offset = 0;
    // No need to re-initialize individual symbol members beyond freeing names and resetting count.
}
