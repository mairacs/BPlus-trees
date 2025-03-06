#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "bp_file.h"
#include "record.h"
#include "bp_datanode.h"


BPLUS_DATA_NODE BP_DATA_NODE_CREATE(int blockId){
    BPLUS_DATA_NODE bp_data_info;
    bp_data_info.num_records = (BF_BLOCK_SIZE - sizeof(BPLUS_DATA_NODE)) / sizeof(Record);
    bp_data_info.records_in_use = 0;
    bp_data_info.next_block = 0;
    bp_data_info.block_id = blockId;

    return bp_data_info;
}

void INSERT_AND_SORT(BF_Block *datanode, BPLUS_DATA_NODE* metadata, Record record){
    void *start = BF_Block_GetData(datanode);
    Record *cur_rec = (Record *)(start + sizeof(BPLUS_DATA_NODE));
    Record *pre, *after;

    // Pass over the records in datanode
    for(int i=0; i < metadata->records_in_use; i++){
        cur_rec = (Record *)(start +sizeof(BPLUS_DATA_NODE) + i*sizeof(Record));
        
        // Place in correct position and move everything after towards the right
        if(record.id < cur_rec->id){
            for(int j = metadata->records_in_use; j > i; j--){
                after = (Record *)(start +sizeof(BPLUS_DATA_NODE) + j*sizeof(Record));
                pre = (Record *)(start +sizeof(BPLUS_DATA_NODE) + (j-1)*sizeof(Record));
                memcpy(after, pre, sizeof(Record));                
            }
            memcpy(cur_rec, &record, sizeof(Record));
            metadata->records_in_use++;
        return ;
        }
    }

    // Place at the end
    cur_rec = (Record *)(start +sizeof(BPLUS_DATA_NODE) + metadata->records_in_use *sizeof(Record));
    memcpy(cur_rec, &record, sizeof(Record));
    metadata->records_in_use++;
}