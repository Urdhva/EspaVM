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

void update_flags(uint16_t r)
{
    if(reg[r] == 0){
        reg[R_COND] = FL_ZRO;
    }
    else if (reg[r] >> 15){     //1 is the left most bit - we move the code by 15 bits to get the first bit
        reg[R_COND] = FL_NEG;
    }
    else{
        reg[R_COND] = FL_POS;
    }
}


// ---> SIGN EXTEND
    //basically add 0s to +ve numbers and 1s to -ve numbers
uint16_t sign_extend(uint16_t x, int bit_count)
{
    if((x >> (bit_count - 1)) & 1)
    {
        x |= (0xFFFF << bit_count);
    }
    return x;
}

//all instructions go here
//--------------------------------------------------------------------------------------//


void add(uint16_t instr)
{
    //destination register
    uint16_t r0 = (instr >> 9) & 0x7;
    //what we're doing in the above snippet:
    //we take our 16 bit instruction - right shift by 9 digits
    //we then extract the first 3 digits from the right (because of 0x7)
    //basically our result will be something like
    //001, 010, 110, etc etc

    //first operand
    uint16_t r1 = (instr >> 6) & 0x7;

    //check whether we are in immediate mode
    uint16_t imm_flag = (instr >> 5) & 0x7;
}







//---------------------------------------------------------------------------------------//

//registors will be stored in an array:
//basically short unsigned int
//r_count is the max size of the registry
uint16_t reg[R_COUNT];

//refer contrl flow screeny for order of functions and loops
// '--->' indicates a snippet defined by the tutorial
int main()
{   
    // ---> MEMORY MAPPED REGISTERES
    //registry number 10 is storingn condition flag zro (1 << 1)
    reg[R_COND] = FL_ZRO;

    //this is the default starting position
    enum { PC_START = 0x3000};  //hexadecimal
    reg[R_PC] = PC_START;        //program counter points to default starting position

    // ---> READ IMAGE FILE
    //THE BELOW TO SNIPPETS MIGHT COME UNDER THE WHILE LOOP
    if(__argc < 2){
        printf("lc3 [image-file1] ...\n");
        //causes normal program termination to occur and performs cleanup before exiting
        exit(2);
    }

    // ---> READ IMAGE
    for(int j = 1; j < __argc; ++j)
    {
        if(!read_image(__argv[j]))
        {
            printf("Failed to load image %s\n", __argv[j]);
            exit(1);
        }
    }

    // ---> MAIN LOOP   
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

    // {SHUTDONW}
}