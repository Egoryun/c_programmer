.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    # Expression (3 * 10) / 2
    # RHS of '/' is 2. Evaluate 2:
    movl $2, %eax
    pushq %rax
    # LHS of '/' is (3 * 10). Evaluate (3 * 10):
    #   RHS of '*' is 10. Evaluate 10:
    movl $10, %eax
    pushq %rax
    #   LHS of '*' is 3. Evaluate 3:
    movl $3, %eax
    #   LHS (3) in %eax. Pop RHS (10) into %rbx:
    popq %rbx
    #   Multiply:
    imull %ebx, %eax  # (3*10) result now in %eax
    # LHS of '/' (result of 3*10) is in %eax. Pop RHS of '/' (2) into %rbx:
    popq %rbx
    # Divide:
    cdq
    idivl %ebx   # ((3*10)/2) result in %eax
    popq %rbp
    ret
