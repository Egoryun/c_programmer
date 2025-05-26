#!/bin/bash

echo "Running compiler tests..."

MINICC=./minicc # Assumes minicc is in the project root
INPUT_DIR="tests/input"
EXPECTED_DIR="tests/expected_assembly"
OUTPUT_DIR="tests/output"

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

passed_tests=0
failed_tests=0
total_tests=0

# Function to run a single test case
run_test_case() {
    local test_name="$1"
    local input_c_file="$INPUT_DIR/${test_name}.c"
    local expected_s_file="$EXPECTED_DIR/${test_name}.s"
    local actual_s_file="$OUTPUT_DIR/${test_name}.s"

    total_tests=$((total_tests + 1))
    echo -n "Test Case: $test_name ... "

    # Run the compiler
    "$MINICC" "$input_c_file" -o "$actual_s_file"
    compiler_exit_code=$?

    if [ $compiler_exit_code -ne 0 ]; then
        echo -e "\033[0;31mFAIL\033[0m (Compiler exited with code $compiler_exit_code)"
        failed_tests=$((failed_tests + 1))
        # Optional: Print compiler's stderr if available (e.g., from a log file)
        # if [ -f "${actual_s_file}.err" ]; then
        #     cat "${actual_s_file}.err"
        # fi
        return
    fi

    # Check if expected file exists
    if [ ! -f "$expected_s_file" ]; then
        echo -e "\033[0;33mSKIP\033[0m (Expected assembly file $expected_s_file not found)"
        # Or treat as failure:
        # echo -e "\033[0;31mFAIL\033[0m (Expected assembly file $expected_s_file not found)"
        # failed_tests=$((failed_tests + 1))
        return
    fi

    # Compare the actual output with the expected output
    diff -q "$actual_s_file" "$expected_s_file"
    diff_exit_code=$?

    if [ $diff_exit_code -ne 0 ]; then
        echo -e "\033[0;31mFAIL\033[0m (Output assembly differs)"
        failed_tests=$((failed_tests + 1))
        echo "  Expected: $expected_s_file"
        echo "  Actual:   $actual_s_file"
        echo "  Run 'diff \"$expected_s_file\" \"$actual_s_file\"' for details."
    else
        echo -e "\033[0;32mPASS\033[0m"
        passed_tests=$((passed_tests + 1))
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
# Add more test cases as they are created, e.g.:
# run_test_case "another_test"

echo ""
echo "--- Test Summary ---"
echo "Total tests: $total_tests"
echo -e "\033[0;32mPassed: $passed_tests\033[0m"
echo -e "\033[0;31mFailed: $failed_tests\033[0m"

if [ $failed_tests -ne 0 ]; then
    exit 1
else
    exit 0
fi
