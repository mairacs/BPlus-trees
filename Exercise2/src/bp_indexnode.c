#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "bp_file.h"
#include "record.h"
#include "bp_indexnode.h"


BPLUS_INDEX_NODE BP_INDEX_NODE_CREATE(int blockId){
    BPLUS_INDEX_NODE bp_index_info;
    bp_index_info.keys_in_use = 0;
    bp_index_info.block_id = blockId;
    bp_index_info.block_parent_id = 0;      // Set to -1 if parent == root

    return bp_index_info;

}

void insert_and_sort_index(BF_Block* indexnode, BPLUS_INDEX_NODE *metadata, int key, int pointer){
    void *start = BF_Block_GetData(indexnode);
    int *cur_key = (int *)(start + sizeof(BPLUS_INDEX_NODE));
    int *cur_Left_pointer = (int *)(start + sizeof(BPLUS_INDEX_NODE) + num_keys * sizeof(int));
    int *cur_Right_pointer = (int *)(start + sizeof(BPLUS_INDEX_NODE) + (num_keys + 1) * sizeof(int));
    int *pre, *after;

    // Pass over the keys in indexnode
    for(int i = 0; i < metadata->keys_in_use; i++){
        cur_key = (int *)(start + sizeof(BPLUS_INDEX_NODE) + i*sizeof(int));

        // Place in correct position and move everything after towards the right
        if(key < *cur_key){
            for(int j = metadata->keys_in_use; j>i; j--){
                after = (int *)(start + sizeof(BPLUS_INDEX_NODE) + j*sizeof(int));
                pre = (int *)(start + sizeof(BPLUS_INDEX_NODE) + (j-1)*sizeof(int));
                memcpy(after, pre, sizeof(int));

                // for the right pointer of key
                after = (int *)(start + sizeof(BPLUS_INDEX_NODE) + (num_keys) * sizeof(int) + (j + 1) * sizeof(int)); 
                pre = (int *)(start + sizeof(BPLUS_INDEX_NODE) + (num_keys) * sizeof(int) + (j) * sizeof(int));
                memcpy(after, pre, sizeof(int));
            }

            // Since pointers = keys + 1 we need to transfer the last pointer separately
            memcpy(cur_key, &key, sizeof(int));
            metadata->keys_in_use++;
            cur_key = (int *)(start + sizeof(BPLUS_INDEX_NODE) +(num_keys)*sizeof(int) +  (i+1)*sizeof(int));
            memcpy(cur_key, &pointer, sizeof(int));
            return;
        }
    }

    cur_key = (int *)(start + sizeof(BPLUS_INDEX_NODE) + (metadata->keys_in_use)*sizeof(int));
    memcpy(cur_key, &key, sizeof(int));
    cur_key = (int *)(start + sizeof(BPLUS_INDEX_NODE) +(num_keys)*sizeof(int) +  (metadata->keys_in_use+1)*sizeof(int));
    memcpy(cur_key, &pointer, sizeof(int));
    metadata->keys_in_use++;
    return;
}