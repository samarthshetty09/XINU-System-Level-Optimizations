/* pfintr.S - pfintr */

        .text
        .globl  pfintr, pferrcode
pferrcode:
        .long   0
.equ KERNEL_DS, 0x10

#
# Stack Layout on Entry to Interrupt Handler
# ------------------------------------------
# Stack Top (Lower Memory Address)
# +------------------+
# | %gs              | <- Register Pushed by ISR
# +------------------+
# | ...              |
# +------------------+
# | %eax             | <- Register Pushed by ISR
# +------------------+
# | Old SS           | <- Pushed by CPU (assumption: priviledge level change from Ring 3 to Ring 0)
# +------------------+
# | Old ESP          | <- Pushed by CPU (assumption: priviledge level change from Ring 3 to Ring 0)
# +------------------+
# | EFLAGS           | <- Pushed by CPU
# +------------------+
# | CS               | <- Pushed by CPU
# +------------------+
# | EIP              | <- Pushed by CPU
# +------------------+
# | Error Code       | <- Pushed by CPU
# +------------------+
# Stack Bottom (Higher Memory Address)
#

pfintr:
        # Save general-purpose registers
        pushl   %eax
        pushl   %ebx
        pushl   %ecx
        pushl   %edx
        pushl   %esi
        pushl   %edi
        pushl   %ebp

        # Save segment registers
        pushl   %ds
        pushl   %es
        pushl   %fs
        pushl   %gs

        # Load kernel data segment
        movw    $KERNEL_DS, %ax
        movw    %ax, %ds
        movw    %ax, %es

        # Get the error code from the stack
        movl    64(%esp), %eax 
        movl    %eax, pferrcode

        # Call the C page fault handler
        call    pfint

        # Restore segment registers
        popl    %gs
        popl    %fs
        popl    %es
        popl    %ds

        # Restore general-purpose registers
        popl    %ebp
        popl    %edi
        popl    %esi
        popl    %edx
        popl    %ecx
        popl    %ebx
        popl    %eax

        addl    $4, %esp          # Remove error code from stack

        iret                       # Return from interrupt
