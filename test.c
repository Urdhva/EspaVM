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
    FL_POS = 1 << 0,    //P
    FL_ZRO = 1 << 1,    //Z
    FL_NEG = 1 << 2,    //N
};

//registors will be stored in an array:
__UINT16_TYPE__ reg[R_COUNT];

int main()
{   
        
}