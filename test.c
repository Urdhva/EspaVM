// #include <stdio.h>
#define MEMORY_MAX (1 << 16)
//unsigned integer of 16 bits
__UINT16_TYPE__ memory[MEMORY_MAX];

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
    OP_BR = 0,      //branch
    OP_ADD,         //ADD
    OP_LD,          //load
    OP_ST,          //store
    OP_JSR,         //jump registor
    OP_AND,         //bitwsie and
    OP_LDR,         //load registor
    OP_STR,         //store registor
    OP_RTI,         //unusued
    OP_NOT,         //bitwise not
    OP_LDI,         //load indirect
    OP_STI,         //store indirect
    OP_JMP,         //jump
    OP_RES,         //reserved (unusued)
    OP_LEA,         //load effective address
    OP_TRAP         //execute trap
};

//registors will be stored in an array:
__UINT16_TYPE__ reg[R_COUNT];

int main(){
    printf("Helo");



    return 0;
}