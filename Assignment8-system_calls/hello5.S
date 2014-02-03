#Elie Weintraub 
#OS - PS 8
#hel1o5.S: "hello, world" in assembly language for Linux x86 32 bit architecture

.globl  _start
.text
_start:

# Write system call:

        movl    $4, %eax           #put system call number (sys_write) in %eax
		movl    $1, %ebx           #put file descriptor in %ebx (argument 1)
        movl    $msg, %ecx         #put string to write in %ecx (argument 2)
		movl    $len, %edx         #put length of string in %edx (argument 3)
        int     $0x80              #make the system call

# Exit system call:

        movl    $1, %eax           #put system call number (sys_exit) in %eax
		movl    $0, %ebx           #put exit code (0) in %ebx (argument 1)
        int     $0x80              #make the system call
		
.data
msg:
        .ascii  "Hello, world!\n"  #string to write
        len =   . - msg            #length of string