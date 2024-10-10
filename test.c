#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <Windows.h>
#include <conio.h>
#define MEMORY_MAX (1 << 16)


HANDLE hStdin = INVALID_HANDLE_VALUE;
DWORD fdwMode, fdwOldMode;

void disable_input_buffering()
{
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hStdin, &fdwOldMode); /* save old mode */
    fdwMode = fdwOldMode
            ^ ENABLE_ECHO_INPUT  /* no input echo */
            ^ ENABLE_LINE_INPUT; /* return when one or
                                    more characters are available */
    SetConsoleMode(hStdin, fdwMode); /* set new mode */
    FlushConsoleInputBuffer(hStdin); /* clear buffer */
}

void restore_input_buffering()
{
    SetConsoleMode(hStdin, fdwOldMode);
}

uint16_t check_key()
{
    return WaitForSingleObject(hStdin, 1000) == WAIT_OBJECT_0 && _kbhit();
}


//unsigned integer of 16 bits
uint16_t memory[MEMORY_MAX];

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

enum Condition_Flags
{
    //(n << k): shift 'n' by 'k' bits
    FL_POS = 1 << 0,    //P
    FL_ZRO = 1 << 1,    //Z     note: translates to "string is not null" in bash
    FL_NEG = 1 << 2,    //N     note: translates to "string is null" in bash
};

//registors will be stored in an array:
//basically short unsigned int
//r_count is the max size of the registry
uint16_t reg[R_COUNT];

int main()
{   

    /*
        Load Arguments
        Setup
    */

    //registry number 10 is storingn condition flag zro (1 << 1)
    reg[R_COND] = FL_ZRO;

    //this is the default starting position
    enum { PC_START = 0x3000};  //hexadecimal
    reg[R_PC] = PC_START;        //program counter points to default starting position

    int running = 1;

    while(running)
    {
        /*Fetch*/
        
        uint16_t instr = mem_read(reg[R_PC]++);
        uint16_t op = instr >> 12;  //right shift 12 bits


        //the order here is different because we go throught the instructions
        //that are most likely to occur.
        switch(op)
        {
            case OP_ADD:
                // @{ADD}
                break;
            case OP_AND:
                // @{AND}
                break;
            case OP_NOT:
                // @{NOT}
                break;
            case OP_BR:
                // @{BR}
                break;
            case OP_JMP:
                // @{JMP}
                break;
            case OP_JSR:
                // @{JSR}
                break;
            case OP_LD:
                // @{LD}
                break;
            case OP_LDI:
                // @{LDI}
                break;
            case OP_LDR:
                // @{LDR}
                break;
            case OP_LEA:
                // @{LEA}
                break;
            case OP_ST:
                // @{ST}
                break;
            case OP_STI:
                // @{STI}
                break;
            case OP_STR:
                // @{STR}
                break;
            case OP_TRAP:
                // @{TRAP}
                break;
            case OP_RES:
            case OP_RTI:
            default:
                // @{BAD OPCODE}
                break;
        }
    }
}