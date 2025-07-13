#ifndef TABLE_H
#define TABLE_H

#include "common.h"
#include "pager.h"

struct Table {
    Pager* pager;
    uint32_t root_page_num;
};

struct Cursor {
    Table* table;
    uint32_t page_num;
    uint32_t cell_num;
    bool end_of_table;
};

Table* db_open(const std::string& filename);
void db_close(Table* table);
Cursor* table_start(Table* table);
Cursor* table_find(Table* table, uint32_t key);
void* cursor_value(Cursor* cursor);
void cursor_advance(Cursor* cursor);

#endif // TABLE_H