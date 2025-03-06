#ifndef BP_DATANODE_H
#define BP_DATANODE_H
#include <record.h>
#include <bf.h>
#include <bp_file.h>

// Our struct for datanodes
typedef struct {

    int records_in_use;                             // how many records the datanode contains
    int num_records;                                // the maximum of how many records the datanode can contains
    int next_block;                                 // pointer to the next datanode
    int block_id;                                   // id of the datanode
    
} BPLUS_DATA_NODE;

BPLUS_DATA_NODE BP_DATA_NODE_CREATE(int blockId);
void INSERT_AND_SORT(BF_Block *datanode, BPLUS_DATA_NODE* metadata, Record record);

#endif