The name **Espa** comes from the K-pop group Aespa (don't mind the name)

LC-3 Emulator

This project implements a basic emulator for the LC-3 (Little Computer 3), a simulated computer architecture often used in computer science education. This emulator reads binary files containing LC-3 machine code, loads them into memory, and executes them instruction-by-instruction, emulating the LC-3's 16-bit architecture.

Features

Instruction Execution: Supports the full set of LC-3 opcodes (e.g., arithmetic, logic, load/store, and branching operations).
Memory Management: Manages up to 2^16 (65,536) memory locations.
Input/Output Operations: Implements LC-3 TRAP routines for character input/output.
Keyboard Buffering (Windows): Disables and restores keyboard buffering to handle real-time console input on Windows.

File Structure

ConsoleBuffer: Disables and restores console buffering (on Windows) for real-time input handling.
Memory Class: Manages memory read and write operations, with special handling for keyboard input.
LC3Emulator Class: Loads binary files into memory and handles byte-swapping for cross-platform compatibility.

Opcode Implementations: Each opcode (instruction) is implemented as a function (e.g., add, branch, trap) and manages registers and flags as per LC-3 specifications.
Usage

Building

To build the emulator, compile the code using a C++ compiler.

Running

Run the emulator with an LC-3 binary image file:


lc3 [image-file1] ...
Example:

bash
Copy code
lc3 my_program.obj


Controls

Input: The emulator supports character input for GETC and IN trap routines.
Halt: The program halts execution upon encountering the HALT trap.

Code Structure
Registers and Condition Flags: Defines registers (R_R0, R_PC, etc.) and condition flags (FL_POS, FL_ZRO, FL_NEG) to track state.

Instruction Functions: Each LC-3 instruction (e.g., ADD, AND, LD, ST, etc.) has its own function, which performs operations based on the instruction's bitwise-encoded arguments.
Main Loop: The main loop fetches, decodes, and executes instructions, updating the PC register until the program halts.

LC-3 Instruction Set

The emulator supports the following LC-3 instructions:

Arithmetic: ADD, AND, NOT
Load/Store: LD, LDI, LDR, ST, STI, STR, LEA
Control Flow: BR, JMP, JSR
Trap Routines: TRAP (supports GETC, OUT, PUTS, IN, HALT)
Platform-Specific Notes

Windows

The ConsoleBuffer class disables console buffering for Windows systems to manage real-time input, restoring it on exit.

Linux/MacOS

Console buffering is not modified, and input is handled with standard I/O functions.
Troubleshooting

Unknown Opcode: The emulator stops execution if it encounters an unknown opcode.
Invalid Memory Address: Writing or reading beyond the memory limit will trigger an error.
