#include "btree.h"

void initialize_leaf_node(void* node) {
    set_node_type(node, NODE_LEAF);
    set_node_root(node, false);
    *leaf_node_num_cells(node) = 0;
    *leaf_node_next_leaf(node) = 0; // 0 represents no sibling
    *node_parent(node) = 0;
}

void initialize_internal_node(void* node) {
    set_node_type(node, NODE_INTERNAL);
    set_node_root(node, false);
    *internal_node_num_keys(node) = 0;
    *node_parent(node) = 0;
}
// ... (rest of the file is unchanged)
uint32_t* internal_node_child(void* node, uint32_t child_num) {
    uint32_t num_keys = *internal_node_num_keys(node);
    if (child_num > num_keys) {
        std::cerr << "Tried to access child_num " << child_num << " > num_keys " << num_keys << std::endl;
        exit(EXIT_FAILURE);
    } else if (child_num == num_keys) {
        return internal_node_right_child(node);
    } else {
        return internal_node_cell(node, child_num);
    }
}

uint32_t internal_node_find_child(void* node, uint32_t key) {
    uint32_t num_keys = *internal_node_num_keys(node);

    uint32_t min_index = 0;
    uint32_t max_index = num_keys;
    while (min_index != max_index) {
        uint32_t index = (min_index + max_index) / 2;
        uint32_t key_to_right = *internal_node_key(node, index);
        if (key_to_right >= key) {
            max_index = index;
        } else {
            min_index = index + 1;
        }
    }
    return min_index;
}

uint32_t get_node_max_key(Pager* pager, void* node) {
    if (get_node_type(node) == NODE_LEAF) {
        return *leaf_node_key(node, *leaf_node_num_cells(node) - 1);
    }
    void* right_child = get_page(pager, *internal_node_right_child(node));
    return get_node_max_key(pager, right_child);
}

void update_internal_node_key(void* node, uint32_t old_key, uint32_t new_key) {
    uint32_t old_child_index = internal_node_find_child(node, old_key);
    *internal_node_key(node, old_child_index) = new_key;
}

void internal_node_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num) {
    void* parent = get_page(table->pager, parent_page_num);
    uint32_t num_keys = *internal_node_num_keys(parent);

    if (num_keys >= INTERNAL_NODE_MAX_CELLS) {
        uint32_t old_max_key_before = get_node_max_key(table->pager, parent);
        uint32_t new_page_num = get_unused_page_num(table->pager);
        void* new_node = get_page(table->pager, new_page_num);
        initialize_internal_node(new_node);
        *node_parent(new_node) = *node_parent(parent);
        *internal_node_num_keys(new_node) = INTERNAL_NODE_MAX_CELLS / 2;
        for (uint32_t i = 0; i < *internal_node_num_keys(new_node); i++) {
            uint32_t old_cell_idx = i + (INTERNAL_NODE_MAX_CELLS + 1) / 2;
            void* old_cell = internal_node_cell(parent, old_cell_idx);
            void* new_cell = internal_node_cell(new_node, i);
            memcpy(new_cell, old_cell, INTERNAL_NODE_CELL_SIZE);
        }
        *internal_node_right_child(new_node) = *internal_node_right_child(parent);
        *internal_node_num_keys(parent) = INTERNAL_NODE_MAX_CELLS / 2;
        *internal_node_right_child(parent) = *internal_node_child(parent, *internal_node_num_keys(parent));
        void* child = get_page(table->pager, child_page_num);
        uint32_t child_max_key = get_node_max_key(table->pager, child);
        uint32_t new_max_key_after_split = get_node_max_key(table->pager, parent);
        if (child_max_key > new_max_key_after_split) {
            internal_node_insert(table, new_page_num, child_page_num);
        } else {
            internal_node_insert(table, parent_page_num, child_page_num);
        }

        if (is_node_root(parent)) {
            create_new_root(table, new_page_num);
        } else {
            uint32_t grandparent_page_num = *node_parent(parent);
            void* grandparent = get_page(table->pager, grandparent_page_num);
            update_internal_node_key(grandparent, old_max_key_before, get_node_max_key(table->pager, parent));
            internal_node_insert(table, grandparent_page_num, new_page_num);
        }
        return;
    }

    void* child = get_page(table->pager, child_page_num);
    uint32_t child_max_key = get_node_max_key(table->pager, child);
    uint32_t index = internal_node_find_child(parent, child_max_key);

    uint32_t right_child_page_num = *internal_node_right_child(parent);
    void* right_child = get_page(table->pager, right_child_page_num);
    if (child_max_key > get_node_max_key(table->pager, right_child)) {
        *internal_node_child(parent, num_keys) = right_child_page_num;
        *internal_node_key(parent, num_keys) = get_node_max_key(table->pager, right_child);
        *internal_node_right_child(parent) = child_page_num;
    } else {
        for (uint32_t i = num_keys; i > index; i--) {
            memcpy(internal_node_cell(parent, i), internal_node_cell(parent, i - 1), INTERNAL_NODE_CELL_SIZE);
        }
        *internal_node_child(parent, index) = child_page_num;
        *internal_node_key(parent, index) = child_max_key;
    }
    *internal_node_num_keys(parent) += 1;
}

void create_new_root(Table* table, uint32_t right_child_page_num) {
    void* root = get_page(table->pager, table->root_page_num);
    void* right_child = get_page(table->pager, right_child_page_num);
    uint32_t left_child_page_num = get_unused_page_num(table->pager);
    void* left_child = get_page(table->pager, left_child_page_num);

    memcpy(left_child, root, PAGE_SIZE);
    set_node_root(left_child, false);

    initialize_internal_node(root);
    set_node_root(root, true);
    *internal_node_num_keys(root) = 1;
    *internal_node_child(root, 0) = left_child_page_num;
    uint32_t left_child_max_key = get_node_max_key(table->pager, left_child);
    *internal_node_key(root, 0) = left_child_max_key;
    *internal_node_right_child(root) = right_child_page_num;

    *node_parent(left_child) = table->root_page_num;
    *node_parent(right_child) = table->root_page_num;
}

void leaf_node_split_and_insert(Cursor* cursor, uint32_t key, Row* value) {
    Table* table = cursor->table;
    void* old_node = get_page(table->pager, cursor->page_num);
    uint32_t old_max_key_before_split = get_node_max_key(table->pager, old_node);
    uint32_t new_page_num = get_unused_page_num(table->pager);
    void* new_node = get_page(table->pager, new_page_num);
    initialize_leaf_node(new_node);

    *node_parent(new_node) = *node_parent(old_node);
    *leaf_node_next_leaf(new_node) = *leaf_node_next_leaf(old_node);
    *leaf_node_next_leaf(old_node) = new_page_num;

    for (int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; i--) {
        void* destination_node;
        if (i >= static_cast<int32_t>(LEAF_NODE_MAX_CELLS / 2)) {
            destination_node = new_node;
        } else {
            destination_node = old_node;
        }
        uint32_t index_within_node = i % (LEAF_NODE_MAX_CELLS / 2);
        void* destination = leaf_node_cell(destination_node, index_within_node);

        if (i == (int32_t)cursor->cell_num) {
            serialize_row(value, leaf_node_value(destination_node, index_within_node));
            *leaf_node_key(destination_node, index_within_node) = key;
        } else if (i > (int32_t)cursor->cell_num) {
            memcpy(destination, leaf_node_cell(old_node, i - 1), LEAF_NODE_CELL_SIZE);
        } else {
            memcpy(destination, leaf_node_cell(old_node, i), LEAF_NODE_CELL_SIZE);
        }
    }

    *(leaf_node_num_cells(old_node)) = LEAF_NODE_MAX_CELLS / 2;
    *(leaf_node_num_cells(new_node)) = (LEAF_NODE_MAX_CELLS + 1) / 2;

    if (is_node_root(old_node)) {
        create_new_root(table, new_page_num);
    } else {
        uint32_t parent_page_num = *node_parent(old_node);
        uint32_t new_max_key = get_node_max_key(table->pager, old_node);
        void* parent = get_page(table->pager, parent_page_num);
        update_internal_node_key(parent, old_max_key_before_split, new_max_key);
        internal_node_insert(table, parent_page_num, new_page_num);
    }
}

void leaf_node_insert(Cursor* cursor, uint32_t key, Row* value) {
    void* node = get_page(cursor->table->pager, cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);

    if (num_cells >= LEAF_NODE_MAX_CELLS) {
        leaf_node_split_and_insert(cursor, key, value);
        return;
    }

    if (cursor->cell_num < num_cells) {
        for (uint32_t i = num_cells; i > cursor->cell_num; i--) {
            memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i - 1), LEAF_NODE_CELL_SIZE);
        }
    }

    *(leaf_node_num_cells(node)) += 1;
    *(leaf_node_key(node, cursor->cell_num)) = key;
    serialize_row(value, leaf_node_value(node, cursor->cell_num));
}

void leaf_node_delete(Cursor* cursor) {
    void* node = get_page(cursor->table->pager, cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);
    uint32_t cell_to_delete = cursor->cell_num;

    for (uint32_t i = cell_to_delete; i < num_cells - 1; i++) {
        memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i + 1), LEAF_NODE_CELL_SIZE);
    }

    *(leaf_node_num_cells(node)) -= 1;
}

void indent(uint32_t level) {
    for (uint32_t i = 0; i < level; i++) {
        std::cout << "  ";
    }
}

void print_tree(Pager* pager, uint32_t page_num, uint32_t indentation_level) {
    void* node = get_page(pager, page_num);
    uint32_t num_keys, child;

    switch (get_node_type(node)) {
        case NODE_LEAF:
            num_keys = *leaf_node_num_cells(node);
            indent(indentation_level);
            std::cout << "- leaf (size " << num_keys << ")" << std::endl;
            for (uint32_t i = 0; i < num_keys; i++) {
                indent(indentation_level + 1);
                std::cout << "- " << *leaf_node_key(node, i) << std::endl;
            }
            break;
        case NODE_INTERNAL:
            num_keys = *internal_node_num_keys(node);
            indent(indentation_level);
            std::cout << "- internal (size " << num_keys << ")" << std::endl;
            for (uint32_t i = 0; i < num_keys; i++) {
                child = *internal_node_child(node, i);
                print_tree(pager, child, indentation_level + 1);
                indent(indentation_level + 1);
                std::cout << "- key " << *internal_node_key(node, i) << std::endl;
            }
            child = *internal_node_right_child(node);
            print_tree(pager, child, indentation_level + 1);
            break;
    }
}
