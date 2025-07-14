#include "btree.h"
#include "table.h"

// --- Internal Function Prototypes ---
static void create_new_root(Table* table, uint32_t right_child_page_num);
static uint32_t get_node_max_key(Pager* pager, void* node);
static void update_internal_node_key(void* node, uint32_t old_key, uint32_t new_key);
static void internal_node_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num);
static void leaf_node_split_and_insert(Table* table, uint32_t page_num, uint32_t cell_num, uint32_t key, Row* value);
static void adjust_root(Table* table);
static void remove_child_from_internal_node(void* node, uint32_t child_page_num);
static uint32_t get_node_child_index(void* parent_node, uint32_t child_page_num);
static void merge_nodes(Table* table, uint32_t node_page_num, uint32_t neighbor_page_num, uint32_t parent_page_num, uint32_t parent_key_index);
static uint32_t internal_node_find_child(void* node, uint32_t key);


// --- Function Implementations ---

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

static uint32_t internal_node_find_child(void* node, uint32_t key) {
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

void leaf_node_insert(Table* table, uint32_t page_num, uint32_t cell_num, uint32_t key, Row* value) {
    void* node = get_page(table->pager, page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);

    if (num_cells >= LEAF_NODE_MAX_CELLS) {
        leaf_node_split_and_insert(table, page_num, cell_num, key, value);
        return;
    }

    if (cell_num < num_cells) {
        for (uint32_t i = num_cells; i > cell_num; i--) {
            memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i - 1), LEAF_NODE_CELL_SIZE);
        }
    }

    *(leaf_node_num_cells(node)) += 1;
    *(leaf_node_key(node, cell_num)) = key;
    serialize_row(value, leaf_node_value(node, cell_num));
}

static void leaf_node_split_and_insert(Table* table, uint32_t page_num, uint32_t cell_num, uint32_t key, Row* value) {
    Pager* pager = table->pager;
    void* old_node = get_page(pager, page_num);
    uint32_t old_max_key_before_split = get_node_max_key(pager, old_node);
    uint32_t new_page_num = get_unused_page_num(pager);
    void* new_node = get_page(pager, new_page_num);
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

        if (i == (int32_t)cell_num) {
            serialize_row(value, leaf_node_value(destination_node, index_within_node));
            *leaf_node_key(destination_node, index_within_node) = key;
        } else if (i > (int32_t)cell_num) {
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
        uint32_t new_max_key = get_node_max_key(pager, old_node);
        void* parent = get_page(pager, parent_page_num);
        update_internal_node_key(parent, old_max_key_before_split, new_max_key);
        internal_node_insert(table, parent_page_num, new_page_num);
    }
}


static void create_new_root(Table* table, uint32_t right_child_page_num) {
    Pager* pager = table->pager;
    void* root = get_page(pager, table->root_page_num);
    void* right_child = get_page(pager, right_child_page_num);
    uint32_t left_child_page_num = get_unused_page_num(pager);
    void* left_child = get_page(pager, left_child_page_num);

    memcpy(left_child, root, PAGE_SIZE);
    set_node_root(left_child, false);

    initialize_internal_node(root);
    set_node_root(root, true);
    *internal_node_num_keys(root) = 1;
    *internal_node_child(root, 0) = left_child_page_num;
    uint32_t left_child_max_key = get_node_max_key(pager, left_child);
    *internal_node_key(root, 0) = left_child_max_key;
    *internal_node_right_child(root) = right_child_page_num;

    *node_parent(left_child) = table->root_page_num;
    *node_parent(right_child) = table->root_page_num;
}

static uint32_t get_node_max_key(Pager* pager, void* node) {
    switch (get_node_type(node)) {
        case NODE_INTERNAL:
            {
            void* right_child = get_page(pager, *internal_node_right_child(node));
            return get_node_max_key(pager, right_child);
            }
        case NODE_LEAF:
            return *leaf_node_key(node, *leaf_node_num_cells(node) - 1);
        default:
            std::cout << "Error: tried to get max key of unknown node type" << std::endl;
            exit(EXIT_FAILURE);
    }
}

static uint32_t get_node_child_index(void* node, uint32_t child_page_num) {
    uint32_t num_keys = *internal_node_num_keys(node);
    for(uint32_t i=0; i < num_keys; ++i) {
        if(*internal_node_child(node, i) == child_page_num) {
            return i;
        }
    }
    if(*internal_node_right_child(node) == child_page_num) {
        return num_keys;
    }
    // Should not happen in a valid tree
    std::cerr << "Could not find child " << child_page_num << " in parent" << std::endl;
    exit(EXIT_FAILURE);
}

static void update_internal_node_key(void* node, uint32_t old_key, uint32_t new_key) {
    uint32_t old_child_index = internal_node_find_child(node, old_key);
    *internal_node_key(node, old_child_index) = new_key;
}

static void internal_node_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num) {
    Pager* pager = table->pager;
    void* parent = get_page(pager, parent_page_num);
    void* child = get_page(pager, child_page_num);
    uint32_t child_max_key = get_node_max_key(pager, child);
    uint32_t index = internal_node_find_child(parent, child_max_key);

    uint32_t original_num_keys = *internal_node_num_keys(parent);
    *internal_node_num_keys(parent) += 1;

    if (original_num_keys >= INTERNAL_NODE_MAX_CELLS) {
        printf("Need to implement splitting internal node\n");
        exit(EXIT_FAILURE);
    }

    uint32_t right_child_page_num = *internal_node_right_child(parent);
    void* right_child = get_page(pager, right_child_page_num);

    if (child_max_key > get_node_max_key(pager, right_child)) {
        /* Replace right child */
        *internal_node_child(parent, original_num_keys) = right_child_page_num;
        *internal_node_key(parent, original_num_keys) = get_node_max_key(pager, right_child);
        *internal_node_right_child(parent) = child_page_num;
    } else {
        /* Make room for new cell */
        for (uint32_t i = original_num_keys; i > index; i--) {
            memcpy(internal_node_cell(parent, i), internal_node_cell(parent, i - 1), INTERNAL_NODE_CELL_SIZE);
        }
        *internal_node_child(parent, index) = child_page_num;
        *internal_node_key(parent, index) = child_max_key;
    }
}

static void remove_child_from_internal_node(void* node, uint32_t child_page_num) {
    uint32_t index = get_node_child_index(node, child_page_num);
    uint32_t num_keys = *internal_node_num_keys(node);
    
    // Shift keys and children
    for(uint32_t i = index; i < num_keys -1; ++i) {
        *internal_node_key(node, i) = *internal_node_key(node, i+1);
        *internal_node_child(node, i+1) = *internal_node_child(node, i+2);
    }
    *internal_node_right_child(node) = *internal_node_child(node, num_keys-1);
    (*internal_node_num_keys(node))--;
}

static void adjust_root(Table* table) {
    Pager* pager = table->pager;
    void* root_node = get_page(pager, table->root_page_num);

    if(get_node_type(root_node) == NODE_INTERNAL && *internal_node_num_keys(root_node) == 0) {
        // If root is an internal node with no keys, its first child becomes the new root.
        uint32_t new_root_page_num = *internal_node_child(root_node, 0);
        void* new_root_node = get_page(pager, new_root_page_num);
        set_node_root(new_root_node, true);
        *node_parent(new_root_node) = 0;
        table->root_page_num = new_root_page_num;
    }
}

static void merge_nodes(Table* table, uint32_t node_page_num, uint32_t neighbor_page_num, uint32_t parent_page_num, uint32_t parent_key_index) {
    Pager* pager = table->pager;
    void* node = get_page(pager, node_page_num);
    void* neighbor_node = get_page(pager, neighbor_page_num);
    void* parent_node = get_page(pager, parent_page_num);

    // Move cells from node to neighbor
    uint32_t neighbor_insertion_index = *leaf_node_num_cells(neighbor_node);
    for (uint32_t i = 0; i < *leaf_node_num_cells(node); i++) {
        memcpy(leaf_node_cell(neighbor_node, neighbor_insertion_index + i), leaf_node_cell(node, i), LEAF_NODE_CELL_SIZE);
    }
    *leaf_node_num_cells(neighbor_node) += *leaf_node_num_cells(node);
    *leaf_node_next_leaf(neighbor_node) = *leaf_node_next_leaf(node);

    remove_child_from_internal_node(parent_node, node_page_num);
    
    // Recursively rebalance parent
    if(!is_node_root(parent_node) && *internal_node_num_keys(parent_node) < 1) { // A non-root internal node must have at least one key
        // Not implemented: Full recursive rebalancing of internal nodes
    }
    adjust_root(table);
}

void btree_delete(Table* table, uint32_t page_num, uint32_t cell_num, uint32_t key) {
    Pager* pager = table->pager;
    void* node = get_page(pager, page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);

    // Remove the cell
    for (uint32_t i = cell_num; i < num_cells - 1; i++) {
        memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i + 1), LEAF_NODE_CELL_SIZE);
    }
    *leaf_node_num_cells(node) -= 1;
    
    if (is_node_root(node)) {
        return; 
    }

    if (*leaf_node_num_cells(node) < LEAF_NODE_MIN_CELLS) {
        // Node is under-utilized, needs rebalancing
        uint32_t parent_page_num = *node_parent(node);
        void* parent_node = get_page(pager, parent_page_num);
        uint32_t child_index = get_node_child_index(parent_node, page_num);

        uint32_t neighbor_page_num;
        uint32_t key_index;
        if(child_index > 0) { // Has left sibling
            neighbor_page_num = *internal_node_child(parent_node, child_index - 1);
            key_index = child_index - 1;
        } else { // No left sibling, must have right one
            neighbor_page_num = *internal_node_child(parent_node, child_index + 1);
            key_index = child_index;
        }
        
        merge_nodes(table, page_num, neighbor_page_num, parent_page_num, key_index);
    }
}


void print_tree(Pager* pager, uint32_t page_num, uint32_t indentation_level) {
    void* node = get_page(pager, page_num);
    uint32_t num_keys, child;

    auto indent = [&](uint32_t level) {
        for (uint32_t i = 0; i < level; i++) {
            std::cout << "  ";
        }
    };

    switch (get_node_type(node)) {
        case NODE_LEAF:
            num_keys = *leaf_node_num_cells(node);
            indent(indentation_level);
            printf("- leaf (size %d)\n", num_keys);
            for (uint32_t i = 0; i < num_keys; i++) {
                indent(indentation_level + 1);
                printf("- %d\n", *leaf_node_key(node, i));
            }
            break;
        case NODE_INTERNAL:
            num_keys = *internal_node_num_keys(node);
            indent(indentation_level);
            printf("- internal (size %d)\n", num_keys);
            for (uint32_t i = 0; i < num_keys; i++) {
                child = *internal_node_child(node, i);
                print_tree(pager, child, indentation_level + 1);
                indent(indentation_level + 1);
                printf("- key %d\n", *internal_node_key(node, i));
            }
            child = *internal_node_right_child(node);
            print_tree(pager, child, indentation_level + 1);
            break;
    }
}
