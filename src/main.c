#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "codegen.h"

// Function to read the entire file into a dynamically allocated string
char* read_file_to_buffer(const char* filepath) {
    FILE* file = fopen(filepath, "rb"); // Open in binary mode to handle all line endings consistently
    if (!file) {
        perror("Error opening input file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (length == -1) {
        perror("Error determining file size");
        fclose(file);
        return NULL;
    }

    char* buffer = (char*)malloc(length + 1);
    if (!buffer) {
        fprintf(stderr, "Error: Could not allocate memory for file buffer.\n");
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, length, file);
    if (bytes_read < (size_t)length) {
        // This can happen if the file size changes between ftell and fread, or actual read error
        fprintf(stderr, "Error reading file: Only read %zu of %ld bytes.\n", bytes_read, length);
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[length] = '\0'; // Null-terminate the string
    fclose(file);
    return buffer;
}

void print_usage(const char* program_name) {
    fprintf(stderr, "Usage: %s <input_file.c> -o <output_file.s>\n", program_name);
    fprintf(stderr, "Example: %s program.c -o program.s\n", program_name);
}

int main(int argc, char* argv[]) {
    const char* input_filepath = NULL;
    const char* output_filepath = NULL;

    // Basic argument parsing
    if (argc == 4) {
        if (strcmp(argv[2], "-o") == 0) {
            input_filepath = argv[1];
            output_filepath = argv[3];
        } else {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    } else if (argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }
    else {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Read Source File
    char* source_code = read_file_to_buffer(input_filepath);
    if (!source_code) {
        return EXIT_FAILURE;
    }

    // Parsing (Lexing is implicitly handled by the parser's first call to get_next_token)
    // The parser needs to be able to call reset_lexer_state() which is in lexer.c but
    // is prototyped in parser.h for this purpose.
    ProgramNode* ast = parse(source_code);
    if (!ast) {
        // The parser should have printed specific error messages.
        fprintf(stderr, "Compilation failed: Parser returned NULL AST.\n");
        free(source_code);
        return EXIT_FAILURE;
    }

    // Code Generation
    FILE* outfile = fopen(output_filepath, "w");
    if (!outfile) {
        perror("Error opening output file");
        free_program_node(ast);
        free(source_code);
        return EXIT_FAILURE;
    }

    generate_assembly(ast, outfile);
    fclose(outfile);

    // Memory Cleanup
    free_program_node(ast);
    free(source_code);

    printf("Compilation successful: %s -> %s\n", input_filepath, output_filepath);
    return EXIT_SUCCESS;
}
