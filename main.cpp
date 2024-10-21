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
            std::cerr << "Error: Unable to restore console mode." << std::endl;
        }
    }

    static uint16_t checkKey()
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
    OP_BR = 0,  //branch    done
    OP_ADD,     // ADD      done
    OP_LD,      //Load      done
    OP_ST,      //Store     donr
    OP_JSR,     //Jump Registor     done
    OP_AND,     //bitwsie and       done
    OP_LDR,     //load registor     done
    OP_STR,     //store registor    done
    OP_RTI,     //unusued
    OP_NOT,     //bitwise not       done
    OP_LDI,     //load indirect     done
    OP_STI,     //store indirect
    OP_JMP,     //jump
    OP_RES,     //reserved (unusued)
    OP_LEA,     //load effective address
    OP_TRAP     //execute trap
};
enum Condition_Flags
{
    FL_POS = 1 << 0,
    FL_ZRO = 1 << 1,
    FL_NEG = 1 << 2
};

uint16_t reg[R_COUNT];

void updateFlags(uint16_t r)
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

uint16_t signExtend(uint16_t x, int bitCount)
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
void branch(uint16_t instr)
{
    uint16_t n = (instr >> 11) & 0x1;// Extract negative flag
    uint16_t z = (instr >> 10) & 0x1;// Extract zero flag
    uint16_t p = (instr >> 9) & 0x1; // Extract positive flag
    uint16_t pcOffset = instr & 0x1FF;// Get 9-bit PC offset
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
void add(uint16_t instr)
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t r1 = (instr >> 6) & 0x7;
    uint16_t immFlag = (instr >> 5) & 0x1;
    if (immFlag)
    {
        uint16_t imm5 = signExtend(instr & 0x1F, 5);
        reg[r0] = reg[r1] + imm5;
    }
    else
    {
        uint16_t r2 = instr & 0x7;
        reg[r0] = reg[r1] + reg[r2];
    }
    updateFlags(r0);
}
//---------------------------------------------------------------------------------------//
//load(c)
//---------------------------------------------------------------------------------------//
void load(uint16_t instr)
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t pcOffset = signExtend(instr & 0x1FF, 9);
    reg[r0] = memory[reg[R_PC] + pcOffset];
    updateFlags(r0);
}
//---------------------------------------------------------------------------------------//
//store (c)
//---------------------------------------------------------------------------------------//
void store(uint16_t instr)
{
    uint16_t sr = (instr >> 9) & 0x7;
    uint16_t pcOffset = signExtend(instr & 0x1FF, 9);
    memory[reg[R_PC] + pcOffset] = reg[sr];
}
//---------------------------------------------------------------------------------------//
//jump to subroutine or jump registor (this has two modes)
//---------------------------------------------------------------------------------------//
void jsr(uint16_t instr)
{
    reg[R_R7] = reg[R_PC];
    uint16_t flag = (instr >> 11) & 0x1;
    if (flag)
    {
        uint16_t baseR = (instr >> 6) & 0x7;
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
void and_op(uint16_t instr)
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t r1 = (instr >> 6) & 0x7;
    uint16_t immFlag = (instr >> 5) & 0x1;
    if (immFlag)
    {
        uint16_t imm5 = signExtend(instr & 0x1F, 5);
        reg[r0] = reg[r1] & imm5;
    }
    else
    {
        uint16_t r2 = instr & 0x7;
        reg[r0] = reg[r1] & reg[r2];
    }
    updateFlags(r0);
}
//---------------------------------------------------------------------------------------//
//load base off set
//---------------------------------------------------------------------------------------//
void loadRegister(uint16_t instr)
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t baseR = (instr >> 6) & 0x7;
    reg[r0] = memory[reg[baseR] + signExtend(instr & 0x3F, 6)];
    updateFlags(r0);
}
//---------------------------------------------------------------------------------------//
//Store Resistor  Created
//---------------------------------------------------------------------------------------//
void storeRegister(uint16_t instr)
{
    uint16_t sr = (instr >> 9) & 0x7;
    uint16_t baseR = (instr >> 6) & 0x7;
    memory[reg[baseR] + signExtend(instr & 0x3F, 6)] = reg[sr];
}
//---------------------------------------------------------------------------------------//
//Logical Not Operator
//---------------------------------------------------------------------------------------//
void not_op(uint16_t instr)
{
    uint16_t r0 = (instr >> 9) & 0x7;  // Destination register (bits [11:9])
    uint16_t r1 = (instr >> 6) & 0x7;  // Source register (bits [8:6])
    reg[r0] = ~reg[r1];  // Perform bitwise NOT on the value in r1 and store the result in r0
    updateFlags(r0);  // Update the condition flags based on the result in r0
    
}
//---------------------------------------------------------------------------------------//
//load Indirect (c)
//---------------------------------------------------------------------------------------//
void loadIndirect(uint16_t instr)
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t pcOffset = signExtend(instr & 0x1FF, 9);
    reg[r0] = memory[memory[reg[R_PC] + pcOffset]];
    updateFlags(r0);
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
    const uint16_t PC_START = 0x3000;
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
        uint16_t instr = memory[reg[R_PC]++];
        uint16_t op = instr >> 12;

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
            //jmp(instr);
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
            //lea(instr);
            break;
        case OP_ST:
            store(instr);
            break;
        case OP_STI:
            //storeIndirect(instr);
            break;
        case OP_STR:
            storeRegister(instr);
            break;
        case OP_TRAP:
            //trap(instr);
            break;
        case OP_RES:
        case OP_RTI:
        default:
            // @{BAD OPCODE}
            break;
        }
    }
}
