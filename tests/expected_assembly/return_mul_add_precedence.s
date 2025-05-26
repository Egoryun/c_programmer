.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    # Expression (4 * 2) + 5
    # RHS of '+' is 5. Evaluate 5:
    movl $5, %eax
    pushq %rax
    # LHS of '+' is (4 * 2). Evaluate (4 * 2):
    #   RHS of '*' is 2. Evaluate 2:
    movl $2, %eax
    pushq %rax
    #   LHS of '*' is 4. Evaluate 4:
    movl $4, %eax
    #   LHS (4) in %eax. Pop RHS (2) into %rbx:
    popq %rbx
    #   Multiply:
    imull %ebx, %eax  # (4*2) result now in %eax
    # LHS of '+' (result of 4*2) is in %eax. Pop RHS of '+' (5) into %rbx:
    popq %rbx
    # Add:
    addl %ebx, %eax   # ((4*2)+5) result in %eax
    popq %rbp
    ret
