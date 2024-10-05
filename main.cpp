//funny how we can simply copy past C's code and it still is valid

//TO CONTINUE - Condition Flags

#include <iostream>
#define MEMORY_MAX (1 << 16)

//basically __UINT16_TYPE__
short unsigned int memory[MEMORY_MAX];

enum Registors
{
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC,
    R_COND,
    R_COUNT
};

enum Opcodes
{
    OP_BR = 0,
    OP_ADD,
    OP_LD,
    OP_ST,
    OP_JSR,
    OP_AND,
    OP_LDR,
    OP_STR,
    OP_RTI,
    OP_NOT,
    OP_LDI,
    OP_STI,
    OP_JMP,
    OP_RES,
    OP_LEA,
    OP_TRAP
};

short unsigned int reg[R_COUNT];

int main()
{
    std::cout << "This is the way\n";

    return 0;
}