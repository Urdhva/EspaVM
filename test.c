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
__UINT16_TYPE__ reg[R_COUNT];

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
        
    }
}