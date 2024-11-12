#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <Windows.h>
#include <conio.h>
#define MEMORY_MAX (1 << 16) // Define the maximum memory size for the LC-3 virtual machine

// Variables to handle input buffering
HANDLE hStdin = INVALID_HANDLE_VALUE;
DWORD fdwMode, fdwOldMode;

// Function to disable input buffering
void disable_input_buffering()
{
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hStdin, &fdwOldMode);     /* save old mode */
    fdwMode = fdwOldMode ^ ENABLE_ECHO_INPUT /* no input echo */
              ^ ENABLE_LINE_INPUT;           /* return when one or
                                                more characters are available */
    SetConsoleMode(hStdin, fdwMode);         /* set new mode */
    FlushConsoleInputBuffer(hStdin);         /* clear buffer */
}

// Function to restore input buffering to its original state
void restore_input_buffering()
{
    SetConsoleMode(hStdin, fdwOldMode);
}

// Check if a key has been pressed
uint16_t check_key()
{
    return WaitForSingleObject(hStdin, 1000) == WAIT_OBJECT_0 && _kbhit();
}

// memory mapped registers
enum MMR
{
    // getting keyboard status is similar to polling
    MR_KBSR = 0xFE00, // keybaord status
    MR_KBDR = 0xFE02, // keyboard data
};

// Trap codes for various system calls
enum TRAP_codes
{
    TRAP_GETC = 0x20,  // get character from the keyboard, not echoed on the terminal
    TRAP_OUT = 0x21,   // Output a character
    TRAP_PUTS = 0x22,  // Output a string
    TRAP_IN = 0x23,    // Input a character
    TRAP_PUTSP = 0x24, // Output a packed string
    TRAP_HALT = 0x25   // Halt the program
};

// our memory storage
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
    R_COND, // Condition flags
    R_COUNT
};

// our register storage
// registors will be stored in an array:
// basically short unsigned int
// r_count is the max size of the registry
uint16_t reg[R_COUNT];

enum Opcodes
{
    OP_BR = 0, // branch            done
    OP_ADD,    // ADD               done
    OP_LD,     // load              done
    OP_ST,     // store             done
    OP_JSR,    // jump registor     done
               // or jump to subroutine
    OP_AND,    // bitwsie and       done
    OP_LDR,    // load registor     done
    OP_STR,    // store registor    done
    OP_RTI,    // unusued           skipped
    OP_NOT,    // bitwise not       done
    OP_LDI,    // load indirect     done
    OP_STI,    // store indirect    done
    OP_JMP,    // jump              done
    OP_RES,    // reserved (unusued)        done
    OP_LEA,    // load effective address    done
    OP_TRAP    // execute trap
};

enum Condition_Flags
{
    //(n << k): shift 'n' by 'k' bits
    FL_POS = 1 << 0, // P
    FL_ZRO = 1 << 1, // Z     note: translates to "string is not null" in bash
    FL_NEG = 1 << 2, // N     note: translates to "string is null" in bash
};

// Signal handler to restore input buffering on interrupt (Ctrl+C)
void handle_interrupt(int signal)
{
    restore_input_buffering();
    printf("\n");
    exit(-2);
}

// ---> SIGN EXTEND
// basically add 0s to +ve numbers and 1s to -ve numbers
uint16_t sign_extend(uint16_t x, int bit_count)
{
    if ((x >> (bit_count - 1)) & 1)
    {
        x |= (0xFFFF << bit_count);
    }
    return x;
}

// x here is a byte
uint16_t swap16(uint16_t x)
{
    return (x << 8) | (x >> 8);
}

void update_flags(uint16_t r)
{
    if (reg[r] == 0)
    {
        reg[R_COND] = FL_ZRO;
    }
    else if (reg[r] >> 15)
    { // 1 is the left most bit - we move the code by 15 bits to get the MSB
        reg[R_COND] = FL_NEG;
    }
    else
    {
        reg[R_COND] = FL_POS;
    }
}

// funciton decleration
// Read a value from memory, handling memory-mapped I/O if needed
uint16_t mem_read(uint16_t address)
{
    if (address == MR_KBSR)
    {
        if (check_key())
        {
            memory[MR_KBSR] = (1 << 15); // Set ready bit
            memory[MR_KBDR] = getchar(); // Store character in KBDR
        }
    }
    return memory[address];
}

// function decleration
// Write a value to memory
void mem_write(uint16_t address, uint16_t val)
{
    memory[address] = val;
}

// all instructions go here
//--------------------------------------------------------------------------------------//

// conditional branch
// SELF WRITTEN
void branch(uint16_t instr)
{
    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
    uint16_t cond_flag = (instr >> 9) & 0x7;
    if (cond_flag & reg[R_COND])
    {
        reg[R_PC] += pc_offset;
    }
}

// add
void add(uint16_t instr)
{
    // destination register
    uint16_t r0 = (instr >> 9) & 0x7;
    // what we're doing in the above snippet:
    // we take our 16 bit instruction - right shift by 9 digits
    // we then extract the first 3 digits from the right (because of 0x7)
    // basically our result will be something like
    // 001, 010, 110, etc etc

    // first operand
    // extract first three bits from 6th bit
    uint16_t r1 = (instr >> 6) & 0x7;

    // check whether we are in immediate mode
    uint16_t imm_flag = (instr >> 5) & 0x1;

    if (imm_flag)
    {
        uint16_t imm5 = sign_extend(instr & 0x1F, 5);
        reg[r0] = reg[r1] + imm5;
    }
    else
    {
        uint16_t r2 = instr & 0x7;
        // we basically take the first three bits (which give us the location of the destinatin register)
        reg[r0] = reg[r1] + reg[r2];
    }

    update_flags(r0);
}

// load
// SELF WRITTEN
void LD(uint16_t instr)
{
    // get destination register
    uint16_t r0 = (instr >> 9) & 0x7;

    uint16_t pc_offset = sign_extend((instr & 0x1FF), 9);

    // we are reading date from (pc + pc_offset) address
    reg[r0] = mem_read(reg[R_PC] + pc_offset);
    update_flags(r0);
}

// store
// SELF WRITTEN
void ST(uint16_t instr)
{
    // NOT THE DESTINATION REGISTER - register where we need to read required data from
    uint16_t sr = (instr >> 9) & 0x7;

    // calculating the PC offset
    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

    mem_write(reg[R_PC] + pc_offset, reg[sr]);

    // this is PC + PC_OFFSET
    // this is where we need to store our data that has been read from sr
    //  uint16_t rs = reg[R_PC] + pc_offset;

    // so contents of rs are to be overwritten with contents of sr
    //  reg[rs] = mem_read(sr);
    // rs is the destination register hence we update it's update flags.
    //(or something like that)
    //  update_flags(rs);
}

// jump to subroutine or jump registor (this has two modes)
// SELF WRITTEN
void JSR(uint16_t instr)
{
    // r7 stores the incremented pc
    // our link back to the calling routine
    // content of pc is stored in the 7th registry
    reg[R_R7] = reg[R_PC];

    // pc is then loaded with the address of the first instruction
    // of the subroutine
    // so basically an undconditional jump (all according to what's written in the manual)
    uint16_t flag = (instr >> 11) & 0x1;

    if (flag)
    {
        // we then get our base register
        // this is the address of the base register
        uint16_t pc_offset = sign_extend(instr & 0x7FF, 11);
        reg[R_PC] += pc_offset;
    }
    else
    {
        uint16_t r1 = (instr >> 6) & 0x7;
        reg[R_PC] = reg[r1];
    }

    // I'm not sure if we need to use the update_flag function here
    // update - no update flag required.
}

// logical and (duh)
// SELF WRITTEN
void AND(uint16_t instr)
{
    // destination registor
    uint16_t r0 = (instr >> 9) & 0x7;
    // source register
    uint16_t r1 = (instr >> 5) & 0x7;

    // checks mode
    uint16_t imm_flag = (instr >> 5) & 0x1;

    if (imm_flag)
    {
        // immediate addressing mode
        uint16_t imm5 = sign_extend(instr & 0x1F, 5);
        // reg[r0] indicates contents of that register
        reg[r0] = reg[r1] & imm5;
    }
    else
    {
        uint16_t r2 = instr & 0x7;
        // i guess we have to do bitwise and?
        reg[r0] = reg[r1] & reg[r2];
    }

    update_flags(r0);
}

// load base + offset (load register)
// SELF WRITTEN
void LDR(uint16_t instr)
{
    // destination register
    uint16_t r0 = (instr >> 9) & 0x7;

    uint16_t baseR = (instr >> 6) & 0x7;

    // uint16_t offset = sign_extend(instr >> 0x3F, 6);

    reg[r0] = mem_read(reg[baseR] + sign_extend(instr & 0x3F, 6));

    update_flags(r0);
}

// store register (store base + offset)
// SELF WRITTEN
void STR(uint16_t instr)
{
    // DESTINATION REGISTER
    uint16_t r0 = (instr >> 9) & 0x7;
    // base register to which we add pc_offset
    uint16_t r1 = (instr >> 6) & 0x7;
    uint16_t offset = sign_extend(instr & 0x3F, 6);
    mem_write(reg[r1] + offset, reg[r0]);
}

// says unused??
// SELF WRITTEN
void RTI(uint16_t instr)
{
    abort();
}

// bitwise complement (bitwise not)
// SELF WRITTEN
void NOT(uint16_t instr)
{
    uint16_t r0 = (instr >> 9) & 0x7;

    uint16_t r1 = (instr >> 6) & 0x7;

    // flip the bits in contents of source register (does that make sense?)
    reg[r0] = ~(reg[r1]);
    update_flags(r0);
}

// load indirect
void LDI(uint16_t instr)
{
    // get destination register
    uint16_t r0 = (instr >> 9) & 0x7;

    // PC offset
    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

    // we are reading address from pc + pc_offset
    // and then we are reading data from that address
    reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset));
    update_flags(r0);
}

// store indirect
// SELF WRITTEN
void STI(uint16_t instr)
{
    // NOT THE DESTINATION REGISTER
    uint16_t sr = (instr >> 9) & 0x7;

    // destination register
    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
    mem_write(mem_read(reg[R_PC] + pc_offset), reg[sr]);
}

// jump
// SELF WRITTEN
void JMP(uint16_t instr)
{
    // base register
    uint16_t r0 = (instr >> 6) & 0x7;

    // okay so apparently it stores data of the registry
    reg[R_PC] = reg[r0];
}

// reserved
// SELF WRITTEN
void RES(uint16_t instr)
{
    // reserved
}

// laod effective address
// SELF WRITTEN
void LEA(uint16_t instr)
{
    // destination register
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
    reg[r0] = reg[R_PC] + pc_offset;
    update_flags(r0);
}

//---------------------------------------------------------------------------------------//
// Instructions end here

// predefined routines to perform common tasks.
// getting input from the keyboard comes under trap routines
// something something - similar to an operating system
// remember - what we are designging is the architecture
// not the operating systemn.
/// we need an operating system for the user to interact with the machine.

// read image file goes here
void read_image_file(FILE *file)
{
    // the origin tells us where the memory to place the image

    // origin is the first 16 bits of the program which tells us
    // where the execution of the program starts from

    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);
    origin = swap16(origin);

    uint16_t max_read = MEMORY_MAX - origin;
    uint16_t *p = memory + origin;
    size_t read = fread(p, sizeof(uint16_t), max_read, file);

    // swap to little endian (whatever that means)
    while (read-- > 0)
    {
        *p = swap16(*p);
        ++p;
    }
}

// read image goes here
int read_image(const char *image_path)
{
    //'rb' is a mode
    FILE *file = fopen(image_path, "rb");
    if (!file)
    {
        return 0;
    }
    read_image_file(file);
    fclose(file);
    return 1;
}

// refer contrl flow screeny for order of functions and loops
//  '--->' indicates a snippet defined by the tutorial
int main(int argc, char *argv[])
{
    signal(SIGINT, handle_interrupt);
    disable_input_buffering();

    // ---> MEMORY MAPPED REGISTERES
    // registry number 10 is storingn condition flag zro (1 << 1)
    reg[R_COND] = FL_ZRO;

    // this is the default starting position
    enum
    {
        PC_START = 0x3000
    }; // hexadecimal
    reg[R_PC] = PC_START; // program counter points to default starting position

    // ---> READ IMAGE FILE
    // THE BELOW TO SNIPPETS MIGHT COME UNDER THE WHILE LOOP
    if (__argc < 2)
    {
        printf("lc3 [image-file1] ...\n");
        // causes normal program termination to occur and performs cleanup before exiting
        exit(2);
    }

    // ---> READ IMAGE
    for (int j = 1; j < __argc; ++j)
    {
        if (!read_image(__argv[j]))
        {
            printf("Failed to load image %s\n", __argv[j]);
            exit(1);
        }
    }

    // ---> MAIN LOOP
    int running = 1;
    while (running)
    {
        /*Fetch*/

        uint16_t instr = mem_read(reg[R_PC]++);
        uint16_t op = instr >> 12; // right shift 12 bits

        // the order here is different because we go throught the instructions
        // that are most likely to occur.
        switch (op)
        {
        case OP_ADD:
            add(instr);
            break;
        case OP_AND:
            AND(instr);
            break;
        case OP_NOT:
            NOT(instr);
            break;
        case OP_BR:
            branch(instr);
            break;
        case OP_JMP:
            JMP(instr);
            break;
        case OP_JSR:
            JSR(instr);
            break;
        case OP_LD:
            LD(instr);
            break;
        case OP_LDI:
            LDI(instr);
            break;
        case OP_LDR:
            LDR(instr);
            break;
        case OP_LEA:
            LEA(instr);
            break;
        case OP_ST:
            ST(instr);
            break;
        case OP_STI:
            STI(instr);
            break;
        case OP_STR:
            STR(instr);
            break;
        case OP_TRAP:
            reg[R_R7] = reg[R_PC];

            switch (instr & 0xFF)
            {
            case TRAP_GETC:
            {
                // read single ASCII value
                reg[R_R0] = (uint16_t)getchar();
                update_flags(R_R0);
                break;
            }
            case TRAP_OUT:
            {
                putc((char)reg[R_R0], stdout);
                fflush(stdout);
                break;
            }
            case TRAP_PUTS:
            {
                uint16_t *c = memory + reg[R_R0];
                while (*c)
                {
                    putc((char)*c, stdout);
                    ++c;
                }
                fflush(stdout);
                break;
            }
            case TRAP_IN:
            {
                printf("Enter a characters: ");
                char c = getchar();
                putc(c, stdout);
                fflush(stdout);
                reg[R_R0] = (uint16_t)c;
                update_flags(R_R0);
                break;
            }
            case TRAP_PUTSP:
            {
                uint16_t *ch = memory + reg[R_R0];
                while (*ch)
                {
                    char char1 = (*ch) & 0xFF;
                    putc(char1, stdout);
                    char char2 = (*ch) >> 8;
                    if (char2)
                        putc(char2, stdout);
                    ++ch;
                }
                fflush(stdout);
                break;
            }
            case TRAP_HALT:
            {
                puts("HALT");
                fflush(stdout);
                running = 0;
                break;
            }
            }
            break;
        case OP_RES:
            RES(instr);
            break;
        case OP_RTI:
            RTI(instr);
            break;
        default:
            // @{BAD OPCODE}
            break;
        }
    }

    restore_input_buffering();
}