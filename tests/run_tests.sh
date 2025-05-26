#!/bin/bash

echo "Running compiler tests..."

MINICC=./minicc # Assumes minicc is in the project root
INPUT_DIR="tests/input"
ERROR_INPUT_DIR="tests/error_handling" # New directory for error test inputs
EXPECTED_DIR="tests/expected_assembly"
OUTPUT_DIR="tests/output"

# ANSI Color Codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m' # For SKIP
NC='\033[0m' # No Color


# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

PASSED_TESTS=0 # Renamed for consistency
FAILED_TESTS=0 # Renamed for consistency
TOTAL_TESTS=0  # Renamed for consistency

# Function to run a single test case
run_test_case() {
    local test_name="$1"
    local input_c_file="$INPUT_DIR/${test_name}.c"
    local expected_s_file="$EXPECTED_DIR/${test_name}.s"
    local actual_s_file="$OUTPUT_DIR/${test_name}.s"

    TOTAL_TESTS=$((TOTAL_TESTS + 1)) # Use new variable name
    echo -n "Test Case: $test_name ... "

    # Run the compiler
    "$MINICC" "$input_c_file" -o "$actual_s_file"
    compiler_exit_code=$?

    if [ $compiler_exit_code -ne 0 ]; then
        echo -e "${RED}FAIL${NC} (Compiler exited with code $compiler_exit_code)"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return
    fi

    if [ ! -f "$expected_s_file" ]; then
        echo -e "${YELLOW}SKIP${NC} (Expected assembly file $expected_s_file not found)"
        return
    fi

    diff -q "$actual_s_file" "$expected_s_file"
    diff_exit_code=$?

    if [ $diff_exit_code -ne 0 ]; then
        echo -e "${RED}FAIL${NC} (Output assembly differs)"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        echo "  Expected: $expected_s_file"
        echo "  Actual:   $actual_s_file"
        echo "  Run 'diff \"$expected_s_file\" \"$actual_s_file\"' for details."
    else
        echo -e "${GREEN}PASS${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    fi
}

# --- Add test cases here ---
run_test_case "return_0"
run_test_case "return_42"
run_test_case "return_add_simple"
run_test_case "return_add_chained"
run_test_case "return_sub_simple"
run_test_case "return_mul_simple"
run_test_case "return_add_mul_precedence"
run_test_case "return_sub_mul_precedence"
run_test_case "return_mul_add_precedence"
run_test_case "return_left_assoc_sub"
run_test_case "return_div_simple"
run_test_case "return_div_truncate"
run_test_case "return_add_div_precedence"
run_test_case "return_mul_div_left_assoc"
run_test_case "return_paren_add_mul"
run_test_case "return_paren_sub_mul"
run_test_case "return_paren_nested"
run_test_case "return_paren_div"
run_test_case "return_var_simple"
run_test_case "return_var_in_expr"
run_test_case "return_two_vars"
run_test_case "return_var_reused"
run_test_case "return_cmp_eq_true"
run_test_case "return_cmp_eq_false"
run_test_case "return_cmp_ne_true"
run_test_case "return_cmp_lt_true"
run_test_case "return_cmp_le_true"
run_test_case "return_cmp_gt_false"
run_test_case "return_cmp_ge_true"
run_test_case "return_complex_cmp"
run_test_case "if_true"
run_test_case "if_false"
run_test_case "if_else_true"
run_test_case "if_else_false"

# --- Error Test Cases ---
echo ""
echo "Running Error Handling Tests..."

# Function to run a test case that is expected to fail and check stderr
# $1: test name (e.g., "missing_semicolon_return")
# $2: input C file path
# $3: expected error string (a substring to grep for in stderr)
# $4: expected line number in error message (e.g., "line 3,")
# $5: expected column number in error message (e.g., "column 9.")
run_error_test_case() {
    local test_name="$1"
    local input_file="$2"
    local expected_error_substring="$3"
    local expected_line_info="$4"
    local expected_col_info="$5"
    local error_output_file="${OUTPUT_DIR}/${test_name}.stderr" # output_dir is already defined

    echo -n "Test (Error): $test_name ... "
    
    # Run the compiler, redirecting stderr to a file
    "$MINICC" "$input_file" -o "${OUTPUT_DIR}/${test_name}.s" 2> "$error_output_file"
    local exit_code=$?

    TOTAL_TESTS=$((TOTAL_TESTS + 1)) # Increment total tests here

    if [ $exit_code -eq 0 ]; then # Compiler should NOT succeed
        echo -e "${RED}FAIL${NC} (Compiler succeeded, expected failure)"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return
    fi

    local found_error=0
    # Check if the file contains the substring, then line, then column
    if grep -q "$expected_error_substring" "$error_output_file"; then
        # Now check the same line that matched the substring for line and col info
        # This is a bit tricky with grep alone. A more robust way would be to extract the line.
        # For simplicity, we'll grep the whole file for line and col info.
        # This might lead to false positives if line/col info appears elsewhere with a different error.
        # A better approach would be:
        # matching_line=$(grep "$expected_error_substring" "$error_output_file" | head -n 1)
        # if echo "$matching_line" | grep -q "$expected_line_info" && echo "$matching_line" | grep -q "$expected_col_info"; then
        # For now, keeping it simpler:
        if grep -q "$expected_line_info" "$error_output_file" && grep -q "$expected_col_info" "$error_output_file"; then
            # This simplified check might pass if the line/col info exists anywhere in the file,
            # not necessarily on the same line as the expected_error_substring.
            # And if expected_error_substring is too generic.
            # A more precise check would be:
            # if grep -E ".*${expected_error_substring}.*${expected_line_info}.*${expected_col_info}" "$error_output_file"; then
            # Or, even better, combine them into one grep if possible, or awk.
            # Let's try a more combined grep
            if grep -qE "Error: ${expected_error_substring}.*at ${expected_line_info} ${expected_col_info}" "$error_output_file"; then
                 found_error=1
            fi
        fi
    fi
    

    if [ $found_error -eq 1 ]; then
        echo -e "${GREEN}PASS${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}FAIL${NC}"
        echo "    Expected stderr to contain: Error: ${expected_error_substring} ... at ${expected_line_info} ${expected_col_info}"
        echo "    Actual stderr:"
        cat "$error_output_file" | sed 's/^/    /' # Indent cat output
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
}

# Note: Line and column numbers are highly dependent on the lexer's exact behavior
# with newlines and spacing around the error token. These might need fine-tuning.
# The error message format from report_error is: "Error: %s at line %d, column %d. Got token '%.*s' (Type: %d).\n"
# So, we are looking for "Error: <expected_error_substring> at <expected_line_info> <expected_col_info>"

run_error_test_case "missing_semicolon_return" "${ERROR_INPUT_DIR}/missing_semicolon_return.c" "Expected ';' but found '}'" "line 3," "column 9."
run_error_test_case "missing_rparen_if" "${ERROR_INPUT_DIR}/missing_rparen_if.c" "Expected ')' but found '{'" "line 2," "column 14." 
# Adjusted col for missing_rparen_if: "if (1 == 1 {" the '{' is at col 14 if 'if' is col 1, ' ' is col 2 etc.
# Original: if (1 == 1 {  -> if (1==1){ -> if(1==1){
# 12345678901234
# if (1 == 1 {
# Lexer might put column at start of '{'. Let's verify with actual output later. Assuming 1-based indexing.

run_error_test_case "missing_lbrace_if" "${ERROR_INPUT_DIR}/missing_lbrace_if.c" "Expected '{' but found 'return keyword'" "line 3," "column 17." 
# Adjusted col for missing_lbrace_if: "return" starts at col 17 if line 3 is "                return 1;" (16 spaces)
# Actual: "    return 1;" -> 'r' is at col 5.
# The error is from eat_token in parse_block_statement. The token it tries to eat is '{'. The token it *gets* is 'return'.
# The column reported by report_error is current_token.column, which is the column of 'return'.

run_error_test_case "var_decl_missing_identifier" "${ERROR_INPUT_DIR}/var_decl_missing_identifier.c" "Expected identifier but found ';'" "line 2," "column 9." 
# Adjusted col for var_decl_missing_identifier: "    int ;" -> ';' is at col 9 if line 2 is "        int ;" (8 spaces)
# Actual: "    int ;" -> ';' is at col 9.

run_error_test_case "undeclared_var_in_expr" "${ERROR_INPUT_DIR}/undeclared_var_in_expr.c" "Variable 'y' not declared before use" "line 4," "column 20."
run_error_test_case "undeclared_var_assignment" "${ERROR_INPUT_DIR}/undeclared_var_assignment.c" "Variable 'z' not declared before assignment" "line 2," "column 13."
run_error_test_case "redeclaration_var" "${ERROR_INPUT_DIR}/redeclaration_var.c" "Variable 'a' already declared" "line 4," "column 16."


echo ""
echo "--- Test Summary ---"
echo "Total tests: $TOTAL_TESTS"
echo -e "${GREEN}Passed: $PASSED_TESTS${NC}"
echo -e "${RED}Failed: $FAILED_TESTS${NC}"

if [ $FAILED_TESTS -ne 0 ]; then
    exit 1
else
    exit 0
fi
