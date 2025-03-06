#ifndef BP_INDEX_NODE_H
#define BP_INDEX_NODE_H
#include <record.h>
#include <bf.h>
#include <bp_file.h>

extern int num_keys;

// Our struct for indexnodes
typedef struct {
    int keys_in_use;                                    // how many keys the index node contains
    int block_id;                                       // id of the index node
    int block_parent_id;                                // id of the parent of the index node

} BPLUS_INDEX_NODE;

BPLUS_INDEX_NODE BP_INDEX_NODE_CREATE(int blockId);
void insert_and_sort_index(BF_Block* indexnode, BPLUS_INDEX_NODE *metadata, int key, int pointer);

#endif