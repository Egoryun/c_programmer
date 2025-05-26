.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    # Expression (10 - 3) - 2
    # RHS of outer '-' is 2. Evaluate 2:
    movl $2, %eax
    pushq %rax
    # LHS of outer '-' is (10 - 3). Evaluate (10 - 3):
    #   RHS of inner '-' is 3. Evaluate 3:
    movl $3, %eax
    pushq %rax
    #   LHS of inner '-' is 10. Evaluate 10:
    movl $10, %eax
    #   LHS (10) in %eax. Pop RHS (3) into %rbx:
    popq %rbx
    #   Subtract:
    subl %ebx, %eax  # (10-3) result now in %eax
    # LHS of outer '-' (result of 10-3) is in %eax. Pop RHS of outer '-' (2) into %rbx:
    popq %rbx
    # Subtract:
    subl %ebx, %eax   # ((10-3)-2) result in %eax
    popq %rbp
    ret
