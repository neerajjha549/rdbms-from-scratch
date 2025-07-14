#include "vm.h"

VirtualMachine::VirtualMachine(Table* table) : table(table) {}

void VirtualMachine::push_row(const Row& row) {
    row_stack.push_back(row);
}

void VirtualMachine::push_int(uint32_t value) {
    int_stack.push_back(value);
}

void VirtualMachine::print_row_vm(const Row& row) {
    std::cout << "(" << row.id << ", " << row.username << ", " << row.email << ")" << std::endl;
}

void VirtualMachine::execute(const std::vector<Bytecode>& program) {
    for (const auto& instruction : program) {
        switch (instruction.opcode) {
            case OpCode::EXECUTE_INSERT: {
                if (row_stack.empty()) {
                    throw std::runtime_error("VM Error: No row on stack for insert.");
                }
                Row row_to_insert = row_stack.back();
                row_stack.pop_back();
                table_insert(table, &row_to_insert);
                break;
            }

            case OpCode::EXECUTE_SELECT: {
                Cursor* cursor = table_start(table);
                Row row;
                while (!(cursor->end_of_table)) {
                    deserialize_row(cursor_value(cursor), &row);
                    print_row_vm(row);
                    cursor_advance(cursor);
                }
                delete cursor;
                std::cout << "Executed." << std::endl;
                break;
            }
            
            case OpCode::EXECUTE_DELETE: {
                 if (int_stack.empty()) {
                    throw std::runtime_error("VM Error: No ID on stack for delete.");
                }
                uint32_t id_to_delete = int_stack.back();
                int_stack.pop_back();
                table_delete(table, id_to_delete);
                break;
            }

            case OpCode::HALT:
                return;
        }
    }
}
