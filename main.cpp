#include <iostream>
#include <windows.h>
#include <conio.h>
//---------------------------------------------------------------------------------------//
#define MEMORY_MAX (1 << 16)
// memory management 
class ConsoleBuffer
{
public:
    static void disableInputBuffering()
    {
        hStdin = GetStdHandle(STD_INPUT_HANDLE);
        if (hStdin == INVALID_HANDLE_VALUE)
        {
            std::cerr << "Error: Unable to get standard input handle." << std::endl;
            return;
        }
        if (!GetConsoleMode(hStdin, &fdwOldMode))
        {
            std::cerr << "Error: Unable to get console mode." << std::endl;
            return;
        }
        fdwMode = fdwOldMode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
        if (!SetConsoleMode(hStdin, fdwMode))
        {
            std::cerr << "Error: Unable to set console mode." << std::endl;
            return;
        }
        if (!FlushConsoleInputBuffer(hStdin))
        {
            std::cerr << "Error: Unable to flush console input buffer." << std::endl;
        }
    }

    static void restoreInputBuffering()
    {
        if (!SetConsoleMode(hStdin, fdwOldMode))
        {
            //similar to the c version, only with an error handling statement
            std::cerr << "Error: Unable to restore console mode." << std::endl;
        }
    }

    static short int checkKey()
    {
        return (WaitForSingleObject(hStdin, 1000) == WAIT_OBJECT_0 && _kbhit()) ? 1 : 0;
    }

private:
    static HANDLE hStdin;
    static DWORD fdwMode, fdwOldMode;
};

HANDLE ConsoleBuffer::hStdin = INVALID_HANDLE_VALUE;
DWORD ConsoleBuffer::fdwMode = 0;
DWORD ConsoleBuffer::fdwOldMode = 0;
//END OF MEMORY MANAGEMENT
//---------------------------------------------------------------------------------------//
//---------------------------------------------------------------------------------------//
//---------------------------------------------------------------------------------------//
// hardware
//---------------------------------------------------------------------------------------//
//---------------------------------------------------------------------------------------//
//---------------------------------------------------------------------------------------//
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
    OP_BR = 0,  //branch    done
    OP_ADD,     // ADD      done
    OP_LD,      //Load      done
    OP_ST,      //Store     donr
    OP_JSR,     //Jump Registor     done
    OP_AND,     //bitwsie and       done
    OP_LDR,     //load registor     done
    OP_STR,     //store registor    done
    OP_RTI,     //unusued           done
    OP_NOT,     //bitwise not       done
    OP_LDI,     //load indirect     done
    OP_STI,     //store indirect    done
    OP_JMP,     //jump              done
    OP_RES,     //reserved          (unusued)       done
    OP_LEA,     //load effective address            done
    OP_TRAP     //execute trap                      done
};
enum Condition_Flags
{
    FL_POS = 1 << 0,
    FL_ZRO = 1 << 1,
    FL_NEG = 1 << 2
};

short unsigned int reg[R_COUNT];

void updateFlags(short unsigned int r)
{
    if (reg[r] == 0)
    {
        reg[R_COND] = FL_ZRO;
    }
    else if (reg[r] >> 15)
    {
        reg[R_COND] = FL_NEG;
    }
    else
    {
        reg[R_COND] = FL_POS;
    }
}

short unsigned int signExtend(short unsigned int x, int bitCount)
{
    if ((x >> (bitCount - 1)) & 1)
    {
        x |= (0xFFFF << bitCount);
    }
    return x;
}
//END OF HARDWARE
//---------------------------------------------------------------------------------------//
//---------------------------------------------------------------------------------------//
//---------------------------------------------------------------------------------------//
// instructions 
//---------------------------------------------------------------------------------------//
//---------------------------------------------------------------------------------------//
//---------------------------------------------------------------------------------------//
//Conditional Branch(updated from Urdhva)
//---------------------------------------------------------------------------------------//
void branch(short unsigned int instr)
{
    short unsigned int n = (instr >> 11) & 0x1;// Extract negative flag
    short unsigned int z = (instr >> 10) & 0x1;// Extract zero flag
    short unsigned int p = (instr >> 9) & 0x1; // Extract positive flag
    short unsigned int pcOffset = instr & 0x1FF;// Get 9-bit PC offset
    // Check if the condition flags match. The branch is taken if:
    // 1. The 'n' flag in the instruction is set AND the condition register (R_COND) has FL_NEG set, OR
    // 2. The 'z' flag in the instruction is set AND R_COND has FL_ZRO set, OR
    // 3. The 'p' flag in the instruction is set AND R_COND has FL_POS set
    if ((n && (reg[R_COND] & FL_NEG)) || (z && (reg[R_COND] & FL_ZRO)) || (p && (reg[R_COND] & FL_POS)))
    {
        reg[R_PC] += signExtend(pcOffset, 9);// Update the program counter (R_PC) by adding the signed offsets
    }
}
//---------------------------------------------------------------------------------------//
//Add(c)
//---------------------------------------------------------------------------------------//
void add(short unsigned int instr)
{
    short unsigned int r0 = (instr >> 9) & 0x7;
    short unsigned int r1 = (instr >> 6) & 0x7;
    short unsigned int immFlag = (instr >> 5) & 0x1;
    if (immFlag)
    {
        short unsigned int imm5 = signExtend(instr & 0x1F, 5);
        reg[r0] = reg[r1] + imm5;
    }
    else
    {
        short unsigned int r2 = instr & 0x7;
        reg[r0] = reg[r1] + reg[r2];
    }
    updateFlags(r0);
}
//---------------------------------------------------------------------------------------//
//load(c)
//---------------------------------------------------------------------------------------//
void load(short unsigned int instr)
{
    short unsigned int r0 = (instr >> 9) & 0x7;
    short unsigned int pcOffset = signExtend(instr & 0x1FF, 9);
    reg[r0] = memory[reg[R_PC] + pcOffset];
    updateFlags(r0);
}
//---------------------------------------------------------------------------------------//
//store (c)
//---------------------------------------------------------------------------------------//
void store(short unsigned int instr)
{
    short unsigned int sr = (instr >> 9) & 0x7;
    short unsigned int pcOffset = signExtend(instr & 0x1FF, 9);
    memory[reg[R_PC] + pcOffset] = reg[sr];
}
//---------------------------------------------------------------------------------------//
//jump to subroutine or jump registor (this has two modes)
//---------------------------------------------------------------------------------------//
void jsr(short unsigned int instr)
{
    reg[R_R7] = reg[R_PC];
    short unsigned int flag = (instr >> 11) & 0x1;
    if (flag)
    {
        short unsigned int baseR = (instr >> 6) & 0x7;
        reg[R_PC] = reg[baseR];
    }
    else
    {
        reg[R_PC] += signExtend(instr & 0x7FF, 11);
    }
}
//---------------------------------------------------------------------------------------//
//Logical And Operation
//---------------------------------------------------------------------------------------//
void and_op(short unsigned int instr)
{
    short unsigned int r0 = (instr >> 9) & 0x7;
    short unsigned int r1 = (instr >> 6) & 0x7;
    short unsigned int immFlag = (instr >> 5) & 0x1;
    if (immFlag)
    {
        short unsigned int imm5 = signExtend(instr & 0x1F, 5);
        reg[r0] = reg[r1] & imm5;
    }
    else
    {
        short unsigned int r2 = instr & 0x7;
        reg[r0] = reg[r1] & reg[r2];
    }
    updateFlags(r0);
}
//---------------------------------------------------------------------------------------//
//load base off set
//---------------------------------------------------------------------------------------//
void loadRegister(short unsigned int instr)
{
    short unsigned int r0 = (instr >> 9) & 0x7;
    short unsigned int baseR = (instr >> 6) & 0x7;
    reg[r0] = memory[reg[baseR] + signExtend(instr & 0x3F, 6)];
    updateFlags(r0);
}
//---------------------------------------------------------------------------------------//
//Store Resistor  Created
//---------------------------------------------------------------------------------------//
void storeRegister(short unsigned int instr)
{
    short unsigned int sr = (instr >> 9) & 0x7;
    short unsigned int baseR = (instr >> 6) & 0x7;
    memory[reg[baseR] + signExtend(instr & 0x3F, 6)] = reg[sr];
}
//---------------------------------------------------------------------------------------//
//RTI
//---------------------------------------------------------------------------------------//
void RTI(short unsigned int instr)
{
    abort();
}
//---------------------------------------------------------------------------------------//
//Logical Not Operator
//---------------------------------------------------------------------------------------//
void not_op(short unsigned int instr)
{
    short unsigned int r0 = (instr >> 9) & 0x7;  // Destination register (bits [11:9])
    short unsigned int r1 = (instr >> 6) & 0x7;  // Source register (bits [8:6])
    reg[r0] = ~reg[r1];  // Perform bitwise NOT on the value in r1 and store the result in r0
    updateFlags(r0);  // Update the condition flags based on the result in r0
    
}
//---------------------------------------------------------------------------------------//
//load Indirect (c)
//---------------------------------------------------------------------------------------//
void loadIndirect(short unsigned int instr)
{
    short unsigned int r0 = (instr >> 9) & 0x7;
    short unsigned int pcOffset = signExtend(instr & 0x1FF, 9);
    reg[r0] = memory[memory[reg[R_PC] + pcOffset]];
    updateFlags(r0);
}
//---------------------------------------------------------------------------------------//
//store Indirect (c)
//---------------------------------------------------------------------------------------//
void storeIndirect(short unsigned int instr) {
    // Source register
    short unsigned int sr = (instr >> 9) & 0x7;
    // PC offset
    short unsigned int pc_offset = signExtend(instr & 0x1FF, 9);
    // Indirect address: read from memory at (PC + offset), then write to memory at that address
    short unsigned int address = memory[reg[R_PC] + pc_offset];
    // Store the value in sr to memory at the indirect address
    memory[address] = reg[sr];
}
//---------------------------------------------------------------------------------------//
// Jump instruction
//---------------------------------------------------------------------------------------//
void jmp(short unsigned int instr) {
    short unsigned int r0 = (instr >> 6) & 0x7; // Base register
    reg[R_PC] = reg[r0]; // Set PC to the value in the base register
}
//---------------------------------------------------------------------------------------//
// Reserved instruction (usually a placeholder)
//---------------------------------------------------------------------------------------//
void res(short unsigned int instr) {
    // Reserved for future use
}
//---------------------------------------------------------------------------------------//
// Load Effective Address
//---------------------------------------------------------------------------------------//
void lea(short unsigned int instr) {
    // Get the destination register
    short unsigned int r0 = (instr >> 9) & 0x7;
    // Compute the effective address and store it in the destination register
    reg[r0] = reg[R_PC] + signExtend(instr & 0x1FF, 9);
    // Update the condition flags for the destination register
    updateFlags(r0);
}
//---------------------------------------------------------------------------------------//
// trap
//---------------------------------------------------------------------------------------//
void trap(short unsigned int instr) {
    short unsigned int trapvector = instr & 0xFF; // Mask the lower 8 bits to get the trap vector
    reg[R_R7] = reg[R_PC]; // Save the current PC to R7
    switch (trapvector) {
        case 0x20: // GETC: Read a single character from the keyboard
            reg[R_R0] = (short unsigned int)getchar();
            updateFlags(R_R0);
            break;

        case 0x21: // OUT: Output a character
            putc((char)reg[R_R0], stdout);
            fflush(stdout);
            break;

        case 0x22: // PUTS: Output a string
            {
                short unsigned int* c = memory + reg[R_R0];
                while (*c) {
                    putc((char)*c, stdout);
                    ++c;
                }
                fflush(stdout);
            }
            break;

        case 0x23: // IN: Prompt for a single character input
            {
                printf("Enter a character: ");
                char c = getchar();
                reg[R_R0] = (short unsigned int)c;
                updateFlags(R_R0);
            }
            break;

        case 0x24: // PUTSP: Output a string of packed characters
            {
                short unsigned int* c = memory + reg[R_R0];
                while (*c) {
                    char char1 = (*c) & 0xFF;
                    putc(char1, stdout);
                    char char2 = (*c) >> 8;
                    if (char2) putc(char2, stdout);
                    ++c;
                }
                fflush(stdout);
            }
            break;

        case 0x25: // HALT: Halt the program
            printf("HALT\n");
            fflush(stdout);
            exit(0);
            break;

        default:
            printf("Unknown TRAP code: 0x%X\n", trapvector);
            break;
    }
}



//---------------------------------------------------------------------------------------//
//---------------------------------------------------------------------------------------//
//---------------------------------------------------------------------------------------//
//END OF INSTRUCTION
//---------------------------------------------------------------------------------------//
//---------------------------------------------------------------------------------------//
//---------------------------------------------------------------------------------------//


//Main code
int main(int argc, char *argv[])
{
    // Initialize memory and registers
    reg[R_COND] = FL_ZRO;
    const short unsigned int PC_START = 0x3000;
    reg[R_PC] = PC_START;

    // Disable input buffering
    ConsoleBuffer::disableInputBuffering();

    // Handle image loading here
    for (int i = 1; i < argc; ++i)
    {
        if (!readImage(argv[i]))
        {
            std::cerr << "Failed to load image " << argv[i] << std::endl;
            return 1;
        }
    }

    // Main loop
    bool running = true;
    while (running)
    {
        // Fetch instruction
        short unsigned int instr = memory[reg[R_PC]++];
        short unsigned int op = instr >> 12;

        switch (op)
        {
        case OP_ADD:
            add(instr);
            break;
        case OP_AND:
            and_op(instr);
            break;
        case OP_NOT:
            not_op(instr);
            break;
        case OP_BR:
            branch(instr);
            break;
        case OP_JMP:
            jmp(instr);
            break;
        case OP_JSR:
            jsr(instr);
            break;
        case OP_LD:
            load(instr);
            break;
        case OP_LDI:
            loadIndirect(instr);
            break;
        case OP_LDR:
            loadRegister(instr);
            break;
        case OP_LEA:
            lea(instr);
            break;
        case OP_ST:
            store(instr);
            break;
        case OP_STI:
            storeIndirect(instr);
            break;
        case OP_STR:
            storeRegister(instr);
            break;
        case OP_TRAP:
            trap(instr);
            break;
        case OP_RES:
            res(instr);
            break;
        case OP_RTI:
            RTI(instr);
            break;
        default:
            // @{BAD OPCODE}
            break;
        }
    }
}
