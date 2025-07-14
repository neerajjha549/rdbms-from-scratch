#ifndef VM_H
#define VM_H

#include "common.h"
#include "table.h"
#include "row.h"

// Simple bytecode instruction set
enum class OpCode {
    EXECUTE_INSERT,
    EXECUTE_SELECT,
    EXECUTE_DELETE,
    HALT
};

struct Bytecode {
    OpCode opcode;
};

class VirtualMachine {
public:
    VirtualMachine(Table* table);
    void execute(const std::vector<Bytecode>& program);

    // Public methods to manipulate the operand stack from outside
    void push_row(const Row& row);
    void push_int(uint32_t value);

private:
    Table* table;
    // A simple stack for operands
    std::vector<Row> row_stack;
    std::vector<uint32_t> int_stack;
    
    // Helper function to print rows, same as in main before
    void print_row_vm(const Row& row);
};

#endif // VM_H
