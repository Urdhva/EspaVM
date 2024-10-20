//funny how we can simply copy past C's code and it still is valid

//TO CONTINUE - Condition Flags

#include <iostream>
#include <windows.h>
#include <conio.h>
#include <cstdint>
#define MEMORY_MAX (1 << 16)

class ConsoleInputBuffer{//translated code from cpp
private:
    HANDLE hStdin;
    DWORD fdwMode, fdwOldMode;

public:
    ConsoleInputBuffer() : hStdin(INVALID_HANDLE_VALUE) {}

    void disableInputBuffering() {
        hStdin = GetStdHandle(STD_INPUT_HANDLE);
        if (hStdin == INVALID_HANDLE_VALUE) {
            std::cerr << "Error: Unable to get standard input handle." << std::endl;
            return;
        }

        if (!GetConsoleMode(hStdin, &fdwOldMode)) {
            std::cerr << "Error: Unable to get console mode." << std::endl;
            return;
        }

        fdwMode = fdwOldMode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
        if (!SetConsoleMode(hStdin, fdwMode)) {
            std::cerr << "Error: Unable to set console mode." << std::endl;
            return;
        }

        if (!FlushConsoleInputBuffer(hStdin)) {
            std::cerr << "Error: Unable to flush console input buffer." << std::endl;
        }
    }

    void restoreInputBuffering() {
        if (!SetConsoleMode(hStdin, fdwOldMode)) {
            std::cerr << "Error: Unable to restore console mode." << std::endl;
        }
    }

    uint16_t checkKey() {
        if (WaitForSingleObject(hStdin, 1000) == WAIT_OBJECT_0 && _kbhit()) {
            return 1; // Key pressed
        }
        return 0; // No key pressed
    }
};

//basically __UINT16_TYPE__
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
    OP_BR = 0,
    OP_ADD,
    OP_LD,
    OP_ST,
    OP_JSR,
    OP_AND,
    OP_LDR,
    OP_STR,
    OP_RTI,
    OP_NOT,
    OP_LDI,
    OP_STI,
    OP_JMP,
    OP_RES,
    OP_LEA,
    OP_TRAP
};

short unsigned int reg[R_COUNT];

int main()
{
    std::cout << "This is the way\n";

    return 0;
}