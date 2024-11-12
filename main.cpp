#include <iostream>
#include <fstream>
#include <stdint.h>
#include <csignal>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#endif
//---------------------------------------------------------------------------------------//
// memory management 
#define MEMORY_MAX (1 << 16)
#define MR_KBSR 0xFE00 // Keyboard status register
#define MR_KBDR 0xFE02 // Keyboard data register
#ifdef _WIN32
class ConsoleBuffer
{
public:
    static void disableInputBuffering()
    {
        hStdin = GetStdHandle(STD_INPUT_HANDLE);
        if (hStdin != INVALID_HANDLE_VALUE && GetConsoleMode(hStdin, &fdwOldMode))
        {
            fdwMode = fdwOldMode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
            SetConsoleMode(hStdin, fdwMode);
            FlushConsoleInputBuffer(hStdin);
        }
    }

    static void restoreInputBuffering()
    {
        if (hStdin != INVALID_HANDLE_VALUE)
        {
            SetConsoleMode(hStdin, fdwOldMode);
        }
    }

    static bool checkKey()
    {
        return WaitForSingleObject(hStdin, 1000) == WAIT_OBJECT_0 && _kbhit();
    }

private:
    static HANDLE hStdin;
    static DWORD fdwMode, fdwOldMode;
};
HANDLE ConsoleBuffer::hStdin = INVALID_HANDLE_VALUE;
DWORD ConsoleBuffer::fdwMode = 0, ConsoleBuffer::fdwOldMode = 0;
//---------------------------------------------------------------------------------------//
// Handling for POSIX systems
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
class ConsoleBuffer
{
public:
    static void disableInputBuffering()
    {
        struct termios newt;
        tcgetattr(STDIN_FILENO, &oldt);  // Get the current terminal settings
        newt = oldt;                     // Save new terminal settings
        newt.c_lflag &= ~(ICANON | ECHO);  // Disable canonical mode and echo
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);  // Apply the new settings immediately
    }

    static void restoreInputBuffering()
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);  // Restore the old terminal settings (no shit, you wanna use teerminal after/0
    }

    static bool checkKey()
    {
        struct timeval tv = {0};
        fd_set fds;
        FD_ZERO(&fds);  // Clear the file descriptor set
        FD_SET(STDIN_FILENO, &fds);  // Add stdin (file descriptor 0) to the set
        tv.tv_sec = 0; // Timeout
        tv.tv_usec = 1000; // Check every 1 millisecond (because yes)

        return select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0;  // Select is used because getchar() halts program
                                                                           // Since STDIN_FILENO is 0, pass 1 as the first argument
                                                                           // Not monitoring for write or exceptional conditions, so pass nullptr to dont care.
    }

private:
    static struct termios oldt;  // Store original terminal settings
};

struct termios ConsoleBuffer::oldt;
#endif

//---------------------------------------------------------------------------------------//
// Memory operations class
class Memory
{
public:
    static void write(short unsigned int address, short unsigned int val)
    {
        if (address < MEMORY_MAX)
        {
            memory[address] = val;
        }
        else
        {
            std::cerr << "Error: Invalid memory address: " << std::hex << address << std::endl;
        }
    }

    static short unsigned int read(short unsigned int address)
    {
        if (address == MR_KBSR)
        {
            if (ConsoleBuffer::checkKey())
            {
                memory[MR_KBSR] = (1 << 15);
                memory[MR_KBDR] = getchar();
            }
        }
        return memory[address];
    }

    // Returns a pointer to the memory array (used by LC3Emulator for file loading)
    static short unsigned int *getMemory()
    {
        return memory;
    }

private:
    static short unsigned int memory[MEMORY_MAX];
};
short unsigned int Memory::memory[MEMORY_MAX] = {0};


// LC3 Emulator class

//predefined routines to perform common tasks.
//getting input from the keyboard comes under trap routines
//something something - similar to an operating system
//remember - what we are designging is the architecture
//not the operating systemn.
///we need an operating system for the user to interact with the machine.


//read image file goes here
class LC3Emulator
{
public:
    static bool readImage(const char *image_path)
    {
        //read image goes here
        //'rb' is a mode
        std::ifstream file(image_path, std::ios::binary);
        if (!file)
        {
            std::cerr << "Failed to load image " << image_path << std::endl;
            return false;
        }
        readImageFile(file);
        return true;
    }

private:
    static void readImageFile(std::ifstream &file)
    {
        //the origin tells us where the memory to place the image

        //origin is the first 16 bits of the program which tells us
        //where the execution of the program starts from
        short unsigned int origin;
        file.read(reinterpret_cast<char *>(&origin), sizeof(origin));
        origin = swap16(origin);

        short unsigned int max_read = MEMORY_MAX - origin;
        short unsigned int *p = Memory::getMemory() + origin;
        size_t read = file.read(reinterpret_cast<char *>(p), max_read * sizeof(short unsigned int)).gcount() / sizeof(short unsigned int);

        //swap to little endian (whatever that means)
        while (read-- > 0)
        {
            *p = swap16(*p);
            ++p;
        }
    }

    static short unsigned int swap16(short unsigned int x) { return (x << 8) | (x >> 8); }
};

/*HANDLE ConsoleBuffer::hStdin = INVALID_HANDLE_VALUE;
DWORD ConsoleBuffer::fdwMode = 0;
DWORD ConsoleBuffer::fdwOldMode = 0;
short unsigned int Memory::memory[MEMORY_MAX] = {0};*/

//END OF MEMORY MANAGEMENT
//---------------------------------------------------------------------------------------//
//---------------------------------------------------------------------------------------//
//---------------------------------------------------------------------------------------//
// hardware
//---------------------------------------------------------------------------------------//
//---------------------------------------------------------------------------------------//
//---------------------------------------------------------------------------------------//
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

short unsigned int reg[R_COUNT] = {0};

void updateFlags(short unsigned int r)
{
    if (reg[r] == 0)
        reg[R_COND] = FL_ZRO;
    else
        reg[R_COND] = (reg[r] >> 15) ? FL_NEG : FL_POS;
}

short unsigned int signExtend(short unsigned int x, int bitCount)
{
    if ((x >> (bitCount - 1)) & 1)
        x |= (0xFFFF << bitCount);
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
    short unsigned int pc_offset = signExtend(instr & 0x1FF, 9);
    short unsigned int cond_flag = (instr >> 9) & 0x7;
    // Check if the condition flags match. The branch is taken if:
    // 1. The 'n' flag in the instruction is set AND the condition register (R_COND) has FL_NEG set, OR
    // 2. The 'z' flag in the instruction is set AND R_COND has FL_ZRO set, OR
    // 3. The 'p' flag in the instruction is set AND R_COND has FL_POS set
    if (cond_flag & reg[R_COND])
    {
        reg[R_PC] += pc_offset;
    }
}
//---------------------------------------------------------------------------------------//
//Add(c)
//---------------------------------------------------------------------------------------//
void add(short unsigned int instr)
{
    short unsigned int dr = (instr >> 9) & 0x7;
    short unsigned int sr1 = (instr >> 6) & 0x7;
    short unsigned int imm_flag = (instr >> 5) & 0x1;

    if (imm_flag)
    {
        short unsigned int imm5 = signExtend(instr & 0x1F, 5);
        reg[dr] = reg[sr1] + imm5;
    }
    else
    {
        short unsigned int sr2 = instr & 0x7;
        reg[dr] = reg[sr1] + reg[sr2];
    }
    updateFlags(dr);
}
//---------------------------------------------------------------------------------------//
//load(c)
//---------------------------------------------------------------------------------------//
void load(short unsigned int instr)
{
    short unsigned int dr = (instr >> 9) & 0x7;
    short unsigned int pc_offset = signExtend(instr & 0x1FF, 9);
    reg[dr] = Memory::read(reg[R_PC] + pc_offset);
    updateFlags(dr);
}
//---------------------------------------------------------------------------------------//
//store (c)
//---------------------------------------------------------------------------------------//
void store(short unsigned int instr)
{
    short unsigned int sr = (instr >> 9) & 0x7;
    short unsigned int pc_offset = signExtend(instr & 0x1FF, 9);
    Memory::write(reg[R_PC] + pc_offset, reg[sr]);
}
//---------------------------------------------------------------------------------------//
//jump to subroutine or jump registor (this has two modes)
//-------------------------------------AKA--------------------------------------------------//
void jsr(short unsigned int instr)
{
    reg[R_R7] = reg[R_PC];
    short unsigned int flag = (instr >> 11) & 1;
    if (flag)
    {
        short unsigned int pc_offset = signExtend(instr & 0x7FF, 11);
        reg[R_PC] += pc_offset;
    }
    else
    {
        short unsigned int base_r = (instr >> 6) & 0x7;
        reg[R_PC] = reg[base_r];
    }
}
//---------------------------------------------------------------------------------------//
//Logical And Operation
//---------------------------------------------------------------------------------------//
void and_op(short unsigned int instr)
{
    short unsigned int dr = (instr >> 9) & 0x7;
    short unsigned int sr1 = (instr >> 6) & 0x7;
    short unsigned int imm_flag = (instr >> 5) & 0x1;

    if (imm_flag)
    {
        short unsigned int imm5 = signExtend(instr & 0x1F, 5);
        reg[dr] = reg[sr1] & imm5;
    }
    else
    {
        short unsigned int sr2 = instr & 0x7;
        reg[dr] = reg[sr1] & reg[sr2];
    }
    updateFlags(dr);
}
//---------------------------------------------------------------------------------------//
//load base off set
//---------------------------------------------------------------------------------------//
void loadRegister(short unsigned int instr)
{
    short unsigned int dr = (instr >> 9) & 0x7;
    short unsigned int base_r = (instr >> 6) & 0x7;
    short unsigned int offset = signExtend(instr & 0x3F, 6);
    reg[dr] = Memory::read(reg[base_r] + offset);
    updateFlags(dr);
}

//---------------------------------------------------------------------------------------//
//Store Resistor  Created
//---------------------------------------------------------------------------------------//
void storeRegister(short unsigned int instr)
{
    short unsigned int sr = (instr >> 9) & 0x7;
    short unsigned int base_r = (instr >> 6) & 0x7;
    short unsigned int offset = signExtend(instr & 0x3F, 6);
    Memory::write(reg[base_r] + offset, reg[sr]);
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
    short unsigned int dr = (instr >> 9) & 0x7;
    short unsigned int sr = (instr >> 6) & 0x7;
    reg[dr] = ~reg[sr];
    updateFlags(dr);
}
//---------------------------------------------------------------------------------------//
//load Indirect (c)
//---------------------------------------------------------------------------------------//
void loadIndirect(short unsigned int instr)
{
    short unsigned int dr = (instr >> 9) & 0x7;
    short unsigned int pc_offset = signExtend(instr & 0x1FF, 9);
    reg[dr] = Memory::read(Memory::read(reg[R_PC] + pc_offset));
    updateFlags(dr);
}
//---------------------------------------------------------------------------------------//
//store Indirect (c)
//---------------------------------------------------------------------------------------//
void storeIndirect(short unsigned int instr){
    // Source register
    short unsigned int sr = (instr >> 9) & 0x7;
    // PC offset
    // Indirect address: read from memory at (PC + offset), then write to memory at that address
    short unsigned int pc_offset = signExtend(instr & 0x1FF, 9);
    // Store the value in sr to memory at the indirect address
    Memory::write(Memory::read(reg[R_PC] + pc_offset), reg[sr]);
}
//---------------------------------------------------------------------------------------//
// Jump instruction
//---------------------------------------------------------------------------------------//
void jmp(short unsigned int instr) {
    short unsigned int base_r = (instr >> 6) & 0x7;
    reg[R_PC] = reg[base_r];
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
    short unsigned int dr = (instr >> 9) & 0x7;
    // Compute the effective address and store it in the destination register
    short unsigned int pc_offset = signExtend(instr & 0x1FF, 9);
    reg[dr] = reg[R_PC] + pc_offset;
    // Update the condition flags for the destination register
    updateFlags(dr);
}
//---------------------------------------------------------------------------------------//
// trap
//---------------------------------------------------------------------------------------//
void trap(short unsigned int instr)
{
    switch (instr & 0xFF)
    {
    case 0x20: // GETC
        reg[R_R0] = (short unsigned int)getchar();
        break;
    case 0x21: // OUT
        putchar((char)reg[R_R0]);
        break;
    case 0x22: // PUTS
    {
        short unsigned int *c = Memory::getMemory() + reg[R_R0];
        while (*c)
            putchar((char)*c++);
    }
    break;
    case 0x23: // IN
        std::cout << "Enter a character: ";
        reg[R_R0] = (short unsigned int)getchar();
        break;
    case 0x25: // HALT
        std::cout << "HALT\n";
        exit(0);
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

//refer contrl flow screeny for order of functions and loops
// '--->' indicates a snippet defined by the tutorial
//Main code
int main(int argc, char *argv[])
{
    signal(SIGINT, [](int)
           { ConsoleBuffer::restoreInputBuffering(); exit(0); });
    ConsoleBuffer::disableInputBuffering();

    // ---> MEMORY MAPPED REGISTERES
    //registry number 10 is storingn condition flag zro (1 << 1)

    const short unsigned int PC_START = 0x3000;// Default starting position
    reg[R_PC] = PC_START;// Program counter starts at default position
    reg[R_COND] = FL_ZRO;// Initial condition flags (ZRO)

    // ---> READ IMAGE FILE
    //THE BELOW TO SNIPPETS MIGHT COME UNDER THE WHILE LOOP
    if (argc < 2)
    {
        std::cerr << "Usage: lc3 [image-file1] ..." << std::endl;
        //causes normal program termination to occur and performs cleanup before exiting
        exit(2);
    }

    // ---> READ IMAGE
    for (int i = 1; i < argc; ++i)
    {
        if (!LC3Emulator::readImage(argv[i]))
        {
            exit(1);
        }
    }

    // ---> MAIN LOOP  
    bool running = true;
    while (running)
    {
        // Fetch instruction
        short unsigned int instr = Memory::read(reg[R_PC]++);
        short unsigned int op = instr >> 12;

        //the order here is different because we go throught the instructions
        //that are most likely to occur.
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
            std::cerr << "Unknown opcode: " << std::hex << op << std::endl;
            running = false;
            break;
        }
    }
    ConsoleBuffer::restoreInputBuffering();
    return 0;
}
