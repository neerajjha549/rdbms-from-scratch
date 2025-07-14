#ifndef TABLE_H
#define TABLE_H

#include "pager.h"

// Table structure now just holds the pager and root page number.
struct Table {
    Pager* pager;
    uint32_t root_page_num;
};

// A cursor points to a location within the B-Tree.
struct Cursor {
    Table* table;
    uint32_t page_num;
    uint32_t cell_num;
    bool end_of_table;
};


Table* db_open(const std::string& filename);
void db_close(Table* table);

void* cursor_value(Cursor* cursor);
void cursor_advance(Cursor* cursor);

Cursor* table_start(Table* table);
Cursor* table_find(Table* table, uint32_t key);
Cursor* leaf_node_find(Table* table, uint32_t page_num, uint32_t key);
Cursor* internal_node_find(Table* table, uint32_t page_num, uint32_t key);

#endif // TABLE_H