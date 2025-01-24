.global zfunction

zfunction:
    pushl   %ebp            # Save base pointer
    movl    %esp, %ebp       # Set base pointer to stack pointer

    movl    8(%ebp), %eax    # Move parameter 'param' into %eax

    # Step 1: Clear bits 20-27
    andl    $0xfffff00f, %eax    # Apply mask to clear bits 20-27

    # Step 2: Shift left by 8 bits
    shll    $8, %eax             # Shift left by 8 bits

    # Step 3: Fill right-most 8 bits with 1s
    orl     $0xff, %eax          # OR with 0xff to set the last 8 bits to 1

    movl    %ebp, %esp       # Restore stack pointer
    popl    %ebp             # Restore base pointer
    ret                      # Return value in %eax
