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
    OP_BR = 0,      //branch            done
    OP_ADD,         //ADD               done
    OP_LD,          //load              done
    OP_ST,          //store             done
    OP_JSR,         //jump registor     done
                    //or jump to subroutine
    OP_AND,         //bitwsie and       done
    OP_LDR,         //load registor     done
    OP_STR,         //store registor    pending approval
    OP_RTI,         //unusued           skipped
    OP_NOT,         //bitwise not       pending approval
    OP_LDI,         //load indirect     done
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

void update_flags(uint16_t r)
{
    if(reg[r] == 0){
        reg[R_COND] = FL_ZRO;
    }
    else if (reg[r] >> 15){     //1 is the left most bit - we move the code by 15 bits to get the MSB
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


//conditional branch
//SELF WRITTEN
void branch(uint16_t instr)
{
    uint16_t n = (instr >> 11) & 0x1;
    uint16_t z = (instr >> 10) & 0x1;
    uint16_t p = (instr >> 9) & 0x1;

    //0xFF is 511 in integer and 1 1111 1111 in binary
    //basically we are taking only the first 9 bits
    uint16_t pc_offset = instr && 0xFF;   

    // 0x1FF; 

    if ((n && (reg[R_COND] & FL_NEG)) || (z && (reg[R_COND] & FL_ZRO)) || (p && (reg[R_COND] & FL_POS)))
    {
        reg[R_PC] += sign_extend(pc_offset, 9);
    }
}

//add
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
    //extract first three bits from 6th bit
    uint16_t r1 = (instr >> 6) & 0x7;

    //check whether we are in immediate mode
    uint16_t imm_flag = (instr >> 5) & 0x1;

    if(imm_flag)
    {
        uint16_t imm5 = sign_extend(instr & 0x1F, 5);
        reg[r0] = reg[r1] + imm5;
    }
    else
    {
        uint16_t r2 = instr & 0x7;
        //we basically take the first three bits (which give us the location of the destinatin register)
        reg[r0] = reg[r1] + reg[r2];
    }

    update_flags(r0);
}

//load
//SELF WRITTEN
void LD(uint16_t instr)
{
    //get destination register
    uint16_t r0 = (instr >> 9) & 0x7;

    uint16_t pc_offset = sign_extend((instr && 0x1FF), 9);

    //we are reading date from (pc + pc_offset) address
    reg[r0] = mem_read(reg[R_PC] + pc_offset);
    update_flags(r0);
}

//store 
//SELF WRITTEN
void ST(uint16_t instr)
{
    //NOT THE DESTINATION REGISTER - register where we need to read required data from
    uint16_t sr = (instr >> 9) & 0x7;

    //calculating the PC offset
    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

    //this is PC + PC_OFFSET    
    //this is where we need to store our data that has been read from sr
    uint16_t rs = reg[R_PC] + pc_offset;

    //so contents of rs are to be overwritten with contents of sr
    reg[rs] = mem_read(sr);
    //rs is the destination register hence we update it's update flags.
    //(or something like that)
    update_flags(rs);
}

//jump to subroutine or jump registor (this has two modes)
//SELF WRITTEN
void JSR(uint16_t instr)
{
    //r7 stores the incremented pc
    //our link back to the calling routine
    //content of pc is stored in the 7th registry
    reg[R_R7] = reg[R_PC];

    //pc is then loaded with the address of the first instruction 
    //of the subroutine
    //so basically an undconditional jump (all according to what's written in the manual)
    uint16_t flag = (instr >> 1) & 0x1;

    if(flag){
        //we then get our base register
        //this is the address of the base register
        reg[R_PC] = (instr >> 6) & 0x7;
    }
    else{
        reg[R_PC] = reg[R_PC] + sign_extend(instr & 0x7FF, 11);
    }

    //I'm not sure if we need to use the update_flag function here
}

//logical and (duh)
//SELF WRITTEN 
void AND(uint16_t instr)
{
    //destination registor
    uint16_t r0 = (instr >> 9) & 0x7;   
    //source register
    uint16_t r1 = (instr >> 5) & 0x7;

    //checks mode
    uint16_t imm_flag = (instr >> 5) & 0x1;

    if(imm_flag)
    {
        //immediate addressing mode
        uint16_t imm5 = sign_extend(instr & 0x1F, 5);
        //reg[r0] indicates contents of that register
        reg[r0] = reg[r1] & imm5;
    }
    else
    {
        uint16_t r2 = instr & 0x7;
        //i guess we have to do bitwise and?
        reg[r0] = reg[r1] & reg[r2];
    }

    update_flags(r0);
}

//load base + offset (load register)
//SELF WRITTEN
void LDR(uint16_t instr)
{
    //destination register
    uint16_t r0 = (instr >> 9) & 0x7;

    uint16_t baseR = (instr >> 6) & 0x7;

    reg[r0] = reg[baseR] + sign_extend(instr & 0x1F, 5);

    update_flags(r0);
}

//store register (store base + offset)
//SELF WRITTEN
void STR(uint16_t instr)
{
    uint16_t sr = (instr >> 9) & 0x7;
    uint16_t baseR = (instr >> 6) & 0x7;
    memory[reg[baseR] + signExtend(instr & 0x3F, 6)] = reg[sr];
}

//says unused??
//SELF WRITTEN
void RTI(uint16_t instr)
{

}

//bitwise complement (bitwise not)
//SELF WRITTEN
void NOT(uint16_t instr)
{
    uint16_t r0 = (instr >> 9) & 0x7;
    
    uint16_t r1 = (instr >> 6) & 0x7;

    //flip the bits in contents of source register (does that make sense?)
    reg[r0] = ~(reg[r1]);
}

//load indirect
void LDI(uint16_t instr)
{
    //get destination register
    uint16_t r0 = (instr >> 9) & 0x7;

    //PC offset
    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

    //we are reading address from pc + pc_offset
    //and then we are reading data from that address
    reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset));
    update_flags(r0);
}

//store indirect
//SELF WRITTEN
void STI(uint16_t instr)
{
    //NOT THE DESTINATION REGISTER
    uint16_t r0 = (instr >> 9) & 0x7;

    //where we need to store our information
    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

    
}

//---------------------------------------------------------------------------------------//
//Instructions end here


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