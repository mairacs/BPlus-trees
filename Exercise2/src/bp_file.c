#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "bp_file.h"
#include "record.h"
#include <bp_datanode.h>
#include <bp_indexnode.h>

#define CALL_BF(call)         \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }
//      return bplus_ERROR;     

int num_keys = (BF_BLOCK_SIZE - sizeof(BPLUS_INDEX_NODE)) / (2*sizeof(int));

int BP_CreateFile(char *fileName) {
  // Creation of file
  CALL_BF(BF_CreateFile(fileName));

  // Access file
  int fd1;
  CALL_BF(BF_OpenFile(fileName, &fd1));

  // Allocate first block
  BF_Block *first_block;
  BF_Block_Init(&first_block);
  CALL_BF(BF_AllocateBlock(fd1, first_block));

  // Initialize first block with metadata
  BPLUS_INFO bp_info;
  bp_info.height = 0;
  bp_info.root_block = 0;
  bp_info.num_blocks = 0;
  memcpy(BF_Block_GetData(first_block), &bp_info, sizeof(BPLUS_INFO));

  // Set as dirty and unpin
  BF_Block_SetDirty(first_block);
  CALL_BF(BF_UnpinBlock(first_block));

  // Clean up and close file when done
  BF_Block_Destroy(&first_block);
  CALL_BF(BF_CloseFile(fd1));

  return 0;
}


BPLUS_INFO* BP_OpenFile(char *fileName, int *file_desc) {
  // Access file
  CALL_BF(BF_OpenFile(fileName, file_desc));

  // Acquire first block
  BF_Block *first_block;
  BF_Block_Init(&first_block);
  CALL_BF(BF_GetBlock(*file_desc, 0, first_block));

  // Access data of first block
  void *data = BF_Block_GetData(first_block);
  BPLUS_INFO *BpInfo = data;

  // Clean up
  BF_Block_Destroy(&first_block);

  return BpInfo;
}


int BP_CloseFile(int file_desc,BPLUS_INFO* info) {
// Acquire first block
  BF_Block *first_block;
  BF_Block_Init(&first_block);
  CALL_BF(BF_GetBlock(file_desc,0,first_block));
  
  // Set as dirty and unpin
  BF_Block_SetDirty(first_block);
  CALL_BF(BF_UnpinBlock(first_block));
  
  // Clean up (first block which was intentionally left in memory) and close file when done
  BF_Block_Destroy(&first_block);
  CALL_BF(BF_CloseFile(file_desc));

  return 0;
}


int BP_InsertEntry(int file_desc,BPLUS_INFO *bplus_info, Record record) {
  // PRINT TEST
  // printf("Value being inserted: %d\n", record.id);
  int rec_block_id;

  // Case 1 : Empty Tree
  if(bplus_info->height == 0){
    BF_Block *first_datanode, *second_datanode;
    void *data;

    // Allocate datanodes
    BF_Block_Init(&first_datanode);
    BF_Block_Init(&second_datanode);
    CALL_BF(BF_AllocateBlock(file_desc, first_datanode));
    CALL_BF(BF_AllocateBlock(file_desc, second_datanode));

    // Create metadata of datanode
    bplus_info->num_blocks++;
    BPLUS_DATA_NODE bp_data_info1 = BP_DATA_NODE_CREATE(bplus_info->num_blocks);

    // Create metadata of datanode
    bplus_info->num_blocks++;
    BPLUS_DATA_NODE bp_data_info2 = BP_DATA_NODE_CREATE(bplus_info->num_blocks);

    bp_data_info1.next_block = 2;
    bp_data_info2.records_in_use ++;

    // Copy the metadata
    data = BF_Block_GetData(first_datanode);
    memcpy(data, &bp_data_info1, sizeof(BPLUS_DATA_NODE));

    // Copy the metadata
    data = BF_Block_GetData(second_datanode);
    memcpy(data, &bp_data_info2, sizeof(BPLUS_DATA_NODE));

    // Copy the record
    data = data + sizeof(BPLUS_DATA_NODE);
    memcpy(data, &record, sizeof(Record));

    // Set as dirty and unpin
    BF_Block_SetDirty(second_datanode);
    BF_Block_SetDirty(first_datanode);
    CALL_BF(BF_UnpinBlock(second_datanode));
    CALL_BF(BF_UnpinBlock(first_datanode));   
    
    // Set up the root
    BF_Block *root_index_node;
    void *key;
    int *key_id;

    // Allocate indexnodes
    BF_Block_Init(&root_index_node);
    CALL_BF(BF_AllocateBlock(file_desc, root_index_node));
    bplus_info->num_blocks++;

    // Set the root of the B+ tree
    bplus_info->root_block = bplus_info->num_blocks;

    // Create metadata of indexnode
    BPLUS_INDEX_NODE bp_index_info = BP_INDEX_NODE_CREATE(bplus_info->num_blocks);
    bp_index_info.keys_in_use ++;

    // Set parent of root (-1 for checks later)
    bp_index_info.block_parent_id = -1;

    // Copy the metadata
    key = BF_Block_GetData(root_index_node);
    memcpy(key, &bp_index_info, sizeof(BPLUS_INDEX_NODE));
    
    // Copy the key
    key = key + sizeof(BPLUS_INDEX_NODE);
    memcpy(key, &(record.id), sizeof(record.id));
    
    // Copy the pointer of the block that points to the index node
    key = (int *)(key + (num_keys) * sizeof(int));
    int rec_block = 1; 
    memcpy(key, &(rec_block), sizeof(int));

    key = (int *)(key + sizeof(int));
    rec_block = 2;
    memcpy(key, &(rec_block), sizeof(int));

    // Set as dirty and unpin
    BF_Block_SetDirty(root_index_node);
    CALL_BF(BF_UnpinBlock(root_index_node));

    // Clean up
    BF_Block_Destroy(&first_datanode);
    BF_Block_Destroy(&second_datanode);
    BF_Block_Destroy(&root_index_node);

    bplus_info->height++;

    return rec_block;
  }

  // Case 2 : Non Empty Tree
  // Step 1 : Traverse to ideal placement (meaning to a certain leaf-datanode)
  BF_Block *current_block;
  BPLUS_INDEX_NODE *index_node;
  int *keys, *pointers;
  void *data;

  BF_Block_Init(&current_block);

  // Start from the root block
  int current_block_id = bplus_info->root_block;

  // "Climb down" the tree with each iteration in order to reach leaf-datanode
  for (int level=0; level <= bplus_info->height - 1; level++) {
    // PRINT TEST
    // printf("Traversal (Current Block number : %d)\n", current_block_id);

    CALL_BF(BF_GetBlock(file_desc, current_block_id, current_block));
    // Starting memory address of current_block
    data = BF_Block_GetData(current_block);

    index_node = (BPLUS_INDEX_NODE *)data;
    // Memory address of the first key of current_block starts after the metadata
    keys = (int *)(data + sizeof(BPLUS_INDEX_NODE));
    /// Memory address of the first pointer of current_block starts after the keys
    pointers = (int *)(data + sizeof(BPLUS_INDEX_NODE) + (num_keys)*sizeof(int));

    // Find the correct next move
    for (int i=1; i <= index_node->keys_in_use ; i++) {
      // This if statement is executed ONLY when the next move is through the very last pointer
      if (i == index_node->keys_in_use) {
        if(record.id < *keys)
          current_block_id = *pointers;
        else{
          pointers = (int *)(data + sizeof(BPLUS_INDEX_NODE) + (num_keys + i)*sizeof(int));
          current_block_id = *pointers;
        }
        if(level != bplus_info->height-1)
          CALL_BF(BF_UnpinBlock(current_block));
        break;
      }

      // Constantly check if next move is towards the "left"
      if (record.id < *keys) {
        current_block_id = *pointers;
        if(level != bplus_info->height-1)
          CALL_BF(BF_UnpinBlock(current_block));
        break;
      }
      
      // If not simply skip over the key
      keys = (int *)(data + sizeof(BPLUS_INDEX_NODE) + i*sizeof(int));
      pointers = (int *)(data + sizeof(BPLUS_INDEX_NODE) + (num_keys + i)*sizeof(int));
    }
  }

  // Step 2 : Insert into leaf node (including any necessary splits)
  BF_Block* current_datanode;
  int datanode_id = *pointers;
  void *data_of_datanode;

  // Get access to desired datanode
  BF_Block_Init(&current_datanode);
  CALL_BF(BF_GetBlock(file_desc, datanode_id, current_datanode));
  data_of_datanode = BF_Block_GetData(current_datanode);
  BPLUS_DATA_NODE * datanode_metadata = (BPLUS_DATA_NODE *)data_of_datanode;

  // check if record id already exists
  for (int i = 0; i<datanode_metadata->records_in_use; i++) {
    Record *print = (Record *)(data_of_datanode +sizeof(BPLUS_DATA_NODE) + i*sizeof(Record));
    if (record.id == print->id) {
      CALL_BF(BF_UnpinBlock(current_block));
      BF_Block_Destroy(&current_block);
      CALL_BF(BF_UnpinBlock(current_datanode));
      BF_Block_Destroy(&current_datanode);
      // PRINT TEST
      // printf("Already inserted %d\n\n", record.id);
      // printf("\n==================\n");
      return -1;
    }
  }

  // Case 2.1 : Datanode has empty space
  // PRINT TEST
  // printf("%d+1 out of %d records filled in current block\n", datanode_metadata->records_in_use, datanode_metadata->num_records);

  if (datanode_metadata->records_in_use != datanode_metadata->num_records) {
    // Insert to datanode through function
    INSERT_AND_SORT(current_datanode, datanode_metadata, record);
    rec_block_id = datanode_metadata->block_id;

    // Set as dirty and unpin
    CALL_BF(BF_UnpinBlock(current_block));
    
    BF_Block_SetDirty(current_datanode);
    CALL_BF(BF_UnpinBlock(current_datanode));

    // Clean up
    BF_Block_Destroy(&current_block);
    BF_Block_Destroy(&current_datanode);
  }
  // Case 2.2 : Datanode is full
  else if (datanode_metadata->records_in_use == datanode_metadata->num_records) {
    // PRINT TEST
    // printf("Split datanode\n");

    // Step 1 : Split datanode
    BF_Block *new_block;
    void *new_data;

    // Allocate datanode
    BF_Block_Init(&new_block);
    CALL_BF(BF_AllocateBlock(file_desc, new_block));

    bplus_info->num_blocks++;
    BPLUS_DATA_NODE bp_data_info = BP_DATA_NODE_CREATE(bplus_info->num_blocks);
    // Take half of the records of full datanode
    bp_data_info.records_in_use = bp_data_info.num_records/2;

    new_data = BF_Block_GetData(new_block);
    memcpy(new_data, &bp_data_info, sizeof(BPLUS_DATA_NODE));

    // Transfer said records
    for (int i = 0 ; i< datanode_metadata->num_records/2; i++) {
        Record *old = (Record *)(data_of_datanode + sizeof(BPLUS_DATA_NODE) + (i + datanode_metadata->num_records/2)*sizeof(Record));
        Record *new = (Record *)(new_data + sizeof(BPLUS_DATA_NODE) + i*sizeof(Record));
        memcpy(new, old, sizeof(Record));
        datanode_metadata->records_in_use--;
    }

    // Copy to next block
    int temp = datanode_metadata->next_block;
    datanode_metadata->next_block = bp_data_info.block_id;
    memcpy(new_data+2*sizeof(int), &temp, sizeof(int));

    // Add record to the correct datanode out of the two created from the split through function
    Record *new = (Record *)(new_data + sizeof(BPLUS_DATA_NODE));
    if (record.id >= new->id) {
      BF_GetBlock(file_desc, bp_data_info.block_id, new_block);
      void *data_of_new_datanode = BF_Block_GetData(new_block);
      BPLUS_DATA_NODE * new_datanode_metadata = (BPLUS_DATA_NODE *)data_of_new_datanode;

      INSERT_AND_SORT(new_block, new_datanode_metadata, record);
      rec_block_id = new_datanode_metadata->block_id;
    }
    else {
      INSERT_AND_SORT(current_datanode, datanode_metadata, record);
      rec_block_id = datanode_metadata->block_id;
    }

    // Case 2.2.1 : indexnode of datanode (parent of datanode) has space
    if (index_node->keys_in_use != num_keys) {
      insert_and_sort_index(current_block, index_node, new->id, bp_data_info.block_id);
      
      // PRINT TEST
      // printf("NEW INDEX %d: ", index_node->block_id);
      // for(int i=0; i<index_node->keys_in_use; i++){
      //   int *newkey = (int *)(data + sizeof(BPLUS_INDEX_NODE) + i*sizeof(int));
      //   printf("%d ", *newkey);
      // }
      // for(int i=0; i<index_node->keys_in_use+1; i++){
      //   int *newkey = (int *)(data + sizeof(BPLUS_INDEX_NODE) + (num_keys)*sizeof(int) + i*sizeof(int));
      //   printf("%d ", *newkey);
      // }
      // printf("\n");
    }
    // Case 2.2.2 : indexnode of datanode is full
    else {
      // Because of the split we need to send a key to the level above
      int *key_sent_to_higher_level;
      int key_above = new->id;
      int pointer_above = bp_data_info.block_id;
      int just_splitted_root=0;

      while (index_node->keys_in_use == num_keys) {
        // PRINT TEST
        // printf("Split indexnode\n");

        // Step 1 : Split indexnode
        BF_Block *new_index;
        void *new_data;

        BF_Block_Init(&new_index);
        CALL_BF(BF_AllocateBlock(file_desc, new_index));

        bplus_info->num_blocks++;
        BPLUS_INDEX_NODE new_index_metadata = BP_INDEX_NODE_CREATE(bplus_info->num_blocks);
        // Take half of the keys of full indexnode
        new_index_metadata.keys_in_use = num_keys/2;
        new_index_metadata.block_parent_id = index_node->block_parent_id;

        new_data = BF_Block_GetData(new_index);
        memcpy(new_data, &new_index_metadata, sizeof(BPLUS_INDEX_NODE));

        // Transfer said keys as well as the pointers
        for (int i = 0; i <  num_keys/2; i++) {
          int *old_key = (int *)(data + sizeof(BPLUS_INDEX_NODE)+ (i+num_keys/2)*sizeof(int));
          int *new_key = (int *)(new_data + sizeof(BPLUS_INDEX_NODE) + i*sizeof(int));
          memcpy(new_key, old_key, sizeof(int));

          int *old_pointer = (int *)(data + sizeof(BPLUS_INDEX_NODE) + num_keys*sizeof(int) + (i+num_keys/2)*sizeof(int));
          int *new_pointer = (int *)(new_data + sizeof(BPLUS_INDEX_NODE) + num_keys*sizeof(int) + i*sizeof(int));
          memcpy(new_pointer, old_pointer, sizeof(int));

          index_node->keys_in_use--;
        }

      
        // Since pointers = keys + 1 we need to transfer the last pointer separately
        int *old_pointer = (int *)(data + sizeof(BPLUS_INDEX_NODE) + num_keys*sizeof(int) + (num_keys)*sizeof(int));
        int *new_pointer = (int *)(new_data + sizeof(BPLUS_INDEX_NODE) + num_keys*sizeof(int) + (num_keys/2)*sizeof(int));
        memcpy(new_pointer, old_pointer, sizeof(int));


        // Add key to the correct indexnode out of the two created from the split through function
        int *newkey = (int *)(new_data + sizeof(BPLUS_INDEX_NODE));
        if (key_above >= *newkey) {
          insert_and_sort_index(new_index, &new_index_metadata, key_above, pointer_above);
        }
        else {
          insert_and_sort_index(current_block, index_node, key_above, pointer_above);
        }

        if (new_index_metadata.keys_in_use > index_node->keys_in_use) {
          key_sent_to_higher_level = (int *)(new_data + sizeof(BPLUS_INDEX_NODE));
          key_above = *key_sent_to_higher_level;
          pointer_above = new_index_metadata.block_id;
          

          for (int i = 1; i <  new_index_metadata.keys_in_use; i++) {
            // Transfer keys
            int *old_key = (int *)(new_data + sizeof(BPLUS_INDEX_NODE)+ i*sizeof(int));
            int *new_key = (int *)(new_data + sizeof(BPLUS_INDEX_NODE) + (i-1)*sizeof(int));
            memcpy(new_key, old_key, sizeof(int));

            // Transfer pointers
            int *old_pointer = (int *)(new_data + sizeof(BPLUS_INDEX_NODE) + num_keys*sizeof(int) + i*sizeof(int));
            int *new_pointer = (int *)(new_data + sizeof(BPLUS_INDEX_NODE) + num_keys*sizeof(int) + (i-1)*sizeof(int));
            memcpy(new_pointer, old_pointer, sizeof(int));
          }

          // Since pointers = keys + 1 we need to transfer the last pointer separately
          int *old_pointer = (int *)(new_data + sizeof(BPLUS_INDEX_NODE) + num_keys*sizeof(int) + (new_index_metadata.keys_in_use)*sizeof(int));
          int *new_pointer = (int *)(new_data + sizeof(BPLUS_INDEX_NODE) + num_keys*sizeof(int) + (new_index_metadata.keys_in_use-1)*sizeof(int));
          memcpy(new_pointer, old_pointer, sizeof(int));

          new_index_metadata.keys_in_use--;

          // Transfer metadata
          memcpy(new_data, &new_index_metadata, sizeof(BPLUS_INDEX_NODE));
        }
        else {
          key_sent_to_higher_level = (int *)(data+ sizeof(BPLUS_INDEX_NODE) + (index_node->keys_in_use - 1)*sizeof(int));
          key_above = *key_sent_to_higher_level;
          pointer_above =new_index_metadata.block_id;

          index_node->keys_in_use--; 
        }
        
        // Special Case : Split has made us reach the root 
        if (index_node->block_parent_id == -1) {
          BF_Block *new_root;
          void *dataroot;

          // Allocate new root
          BF_Block_Init(&new_root);
          CALL_BF(BF_AllocateBlock(file_desc, new_root));

          bplus_info->height++;
          bplus_info->num_blocks++;
          bplus_info->root_block = bplus_info->num_blocks;

          BPLUS_INDEX_NODE root_metadata = BP_INDEX_NODE_CREATE(bplus_info->num_blocks);

          root_metadata.keys_in_use ++;
          // Set parent of root (-1 for checks later)
          root_metadata.block_parent_id = -1;
          index_node->block_parent_id = bplus_info->num_blocks;
          new_index_metadata.block_parent_id = bplus_info->num_blocks;
          new_data = BF_Block_GetData(new_index);
          memcpy(new_data, &new_index_metadata, sizeof(BPLUS_INDEX_NODE));

          // PRINT TEST
          // printf("We have the new root : %d %d %d\n", root_metadata.block_id, root_metadata.block_parent_id, root_metadata.keys_in_use);

          // Copy the metadata
          dataroot = BF_Block_GetData(new_root);
          memcpy(dataroot, &root_metadata, sizeof(BPLUS_INDEX_NODE));
          
          // Copy the key
          int *keyroot = (int *)(dataroot + sizeof(BPLUS_INDEX_NODE));  
          memcpy(keyroot, &(key_above), sizeof(record.id));
          keyroot = (int *)(dataroot + sizeof(BPLUS_INDEX_NODE));

          // Copy the pointer of block that shows the index node
          keyroot = (int *)(dataroot + sizeof(BPLUS_INDEX_NODE)  +(num_keys) * sizeof(int));
          int rec_block = index_node->block_id; 
          memcpy(keyroot, &(rec_block), sizeof(int));

          // Copy the metadata
          keyroot = (int *)(dataroot + sizeof(BPLUS_INDEX_NODE)  +(num_keys) * sizeof(int) + sizeof(int));
          rec_block = new_index_metadata.block_id;
          memcpy(keyroot, &(rec_block), sizeof(int));

          index_node = (BPLUS_INDEX_NODE *)(dataroot);

          // Set as dirty and unpin
          BF_Block_SetDirty(new_root);
          CALL_BF(BF_UnpinBlock(new_root));
          BF_Block_Destroy(&new_root);
          just_splitted_root = 1;
        }
        else{
          // Set as dirty and unpin
          BF_Block_SetDirty(new_index);
          CALL_BF(BF_UnpinBlock(new_index));
          BF_Block_Destroy(&current_block);

          BF_Block_Init(&current_block);          
          CALL_BF(BF_GetBlock(file_desc, index_node->block_parent_id, current_block));
          data = BF_Block_GetData(current_block);
          index_node = (BPLUS_INDEX_NODE *)data;
        }

        // Set as dirty and unpin
        BF_Block_SetDirty(new_index);
        CALL_BF(BF_UnpinBlock(new_index));
        BF_Block_Destroy(&new_index);
      }
      if(!just_splitted_root && index_node->keys_in_use != num_keys) {
        BF_Block *current_index;
        BF_Block_Init(&current_index);
        CALL_BF(BF_GetBlock(file_desc, index_node->block_id, current_index));
        
        new_data = BF_Block_GetData(current_index);
        index_node = (BPLUS_INDEX_NODE *)(new_data);
        insert_and_sort_index(current_index, index_node, key_above, pointer_above);
        // PRINT TEST
        // printf("test for another split %d %d\n", key_above, pointer_above);

        BF_Block_SetDirty(current_index);
        CALL_BF(BF_UnpinBlock(current_index));
        BF_Block_Destroy(&current_index);
      }
    }

    // Set as dirty and unpin
    BF_Block_SetDirty(new_block);
    CALL_BF(BF_UnpinBlock(new_block));
    BF_Block_Destroy(&new_block);

    // Set as dirty and unpin
    BF_Block_SetDirty(current_block);
    CALL_BF(BF_UnpinBlock(current_block));

    BF_Block_SetDirty(current_datanode);
    CALL_BF(BF_UnpinBlock(current_datanode));

    // Clean up
    BF_Block_Destroy(&current_block);
    BF_Block_Destroy(&current_datanode);
  }

  // PRINT TEST
  // printf("\n");
  // int blockId = 1;
  // BF_Block *print;
  // void *data_print;
  // BF_Block_Init(&print);
  // for (int i=0; i<bplus_info->num_blocks; i++) {
  //   CALL_BF(BF_GetBlock(file_desc, blockId, print));
  //   data_print = BF_Block_GetData(print);
  //   BPLUS_DATA_NODE * metadata = (BPLUS_DATA_NODE *)data_print;
  //   if (metadata->block_id == 0) {
  //     break;
  //   }
  //     printf("Block number %2d : ", metadata->block_id);
  //     for(int i = 0; i<metadata->records_in_use; i++){
  //         Record *print = (Record *)(data_print +sizeof(BPLUS_DATA_NODE) + i*sizeof(Record));
  //         printf("%3d ", print->id);
  //     }
  //   blockId = metadata->next_block;
  //   printf("\n");
  // CALL_BF(BF_UnpinBlock(print));
  // }

  return rec_block_id;
}



int BP_GetEntry(int file_desc,BPLUS_INFO *bplus_info, int value,Record** record)
{
  BF_Block *current_block;
  BPLUS_INDEX_NODE *index_node;
  int flag = 0;
  int *keys, *pointers;
  void *data;

  BF_Block_Init(&current_block);
  int current_block_id = bplus_info->root_block;

  // Step 1 : "Climb down" the tree with each iteration in order to reach leaf
  for (int level=0; level <= bplus_info->height - 1; level++) {

    CALL_BF(BF_GetBlock(file_desc, current_block_id, current_block));
    // Starting memory address of current_block
    data = BF_Block_GetData(current_block);

    index_node = (BPLUS_INDEX_NODE *)data;
    // PRINT TEST
    // printf("Traversal (Current index Block number : %d %d\n", current_block_id, index_node->keys_in_use);
    // Memory address of the first key of current_block starts after the metadata
    keys = (int *)(data + sizeof(BPLUS_INDEX_NODE));
    /// Memory address of the first pointer of current_block starts after the keys
    pointers = (int *)(data + sizeof(BPLUS_INDEX_NODE) + (num_keys)*sizeof(int));

    // Find the correct next move
    for (int i=1; i <= index_node->keys_in_use ; i++) {
      // This if statement is executed ONLY when the next move is through the very last pointer
      if (i == index_node->keys_in_use) {
        if(value < *keys)
          current_block_id = *pointers;
        else{
          pointers = (int *)(data + sizeof(BPLUS_INDEX_NODE) + (num_keys + i)*sizeof(int));
          current_block_id = *pointers;
        }
        if(level != bplus_info->height-1)
          CALL_BF(BF_UnpinBlock(current_block));
        break;
      }

      // Constantly check if next move is towards the "left"
      if (value < *keys) {
        current_block_id = *pointers;
        if(level != bplus_info->height-1)
          CALL_BF(BF_UnpinBlock(current_block));
        break;
      }
      
      // If not simply skip over the key
      keys = (int *)(data + sizeof(BPLUS_INDEX_NODE) + i*sizeof(int));
      pointers = (int *)(data + sizeof(BPLUS_INDEX_NODE) + (num_keys + i)*sizeof(int));
    }
  }

  // Step 2 : Search through the datanode
  BF_Block* current_datanode;
  int datanode_id = *pointers;
  void *data_of_datanode;

  // Get access to desired datanode
  BF_Block_Init(&current_datanode);
  CALL_BF(BF_GetBlock(file_desc, datanode_id, current_datanode));
  data_of_datanode = BF_Block_GetData(current_datanode);
  BPLUS_DATA_NODE * datanode_metadata = (BPLUS_DATA_NODE *)data_of_datanode;
  // PRINT TEST
  // printf("Traversal (Current data Block number : %d\n", datanode_id);

  for (int i = 0; i<datanode_metadata->records_in_use; i++) {
    Record *print = (Record *)(data_of_datanode +sizeof(BPLUS_DATA_NODE) + i*sizeof(Record));
    if (value == print->id) {
      *record = print;
      flag = 1;
    }
  }

  // Set as dirty and unpin
  CALL_BF(BF_UnpinBlock(current_block));
  CALL_BF(BF_UnpinBlock(current_datanode));

  // Clean up
  BF_Block_Destroy(&current_block);
  BF_Block_Destroy(&current_datanode);

  if (flag == 1) {
    return 0;
  }

  *record=NULL;
  return -1;
}

void BP_PrintDatanodes(int file_desc, BPLUS_INFO *bplus_info){
  printf("\n");
  int blockId = 1;
  BF_Block *print;
  void *data_print;
  BF_Block_Init(&print);
  for (int i=0; i<bplus_info->num_blocks; i++) {
    CALL_BF(BF_GetBlock(file_desc, blockId, print));
    data_print = BF_Block_GetData(print);
    BPLUS_DATA_NODE * metadata = (BPLUS_DATA_NODE *)data_print;
    if (metadata->block_id == 0) {
      break;
    }
      // PRINT TEST
      printf("Block number %4d : ", metadata->block_id);
      for(int i = 0; i<metadata->records_in_use; i++){
          Record *print = (Record *)(data_print +sizeof(BPLUS_DATA_NODE) + i*sizeof(Record));
          printf("%3d ", print->id);
      }
    blockId = metadata->next_block;
    printf("\n");
  CALL_BF(BF_UnpinBlock(print));
  }
  BF_Block_Destroy(&print);
  printf("\n");
  printf("\n==================\n");

}