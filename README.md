# B+ trees in the course of Database Management Systems Implementation

## Team Information
* Maira Papadopoulou
* Nektarios Pavlakos-Tsimpourakis

## Project Overview
This project implements a B+ Tree for database indexing. It includes efficient insertion, data block and index block management, and metadata handling. The implementation follows structured coding practices, ensuring reliability and optimal performance.

## Data Structures
### B+ Tree Metadata (`BPLUS_INFO`)
Stores general information about the B+ tree file:
```c
typedef struct {
   int height;       // Tree height
   int root_block;   // Root block ID
   int num_blocks;   // Total number of blocks (data + index)
} BPLUS_INFO;
```

### Data Block (`BPLUS_DATA_NODE`)
Stores metadata for data blocks containing records:
```c
typedef struct {
   int records_in_use;  // Number of records in this block
   int num_records;     // Maximum number of records
   int next_block;      // Pointer to the next data block
   int block_id;        // Unique block ID
} BPLUS_DATA_NODE;
```
### Index Block (`BPLUS_INDEX_NODE`)
Stores metadata for index blocks containing keys:
```c
typedef struct {
   int keys_in_use;      // Number of keys in this index block
   int block_id;         // Unique block ID
   int block_parent_id;  // Parent block ID
} BPLUS_INDEX_NODE;
```

## B+ Tree Insertion (`B+_insert`)
```c
B+_insert () {
    Case 1 : If the B+ tree is empty
        Step 1 : Create an index block as the root.
                 Create two data blocks (left and right of the key).

    Case 2 : If the B+ tree is not empty
        Step 1 : Traverse the B+ tree to locate the correct data block.
        Step 2 : Insert the record.

        Case 2.1 : If there is space in the data block, insert the record.
        Case 2.2 : If the data block is full
            Step 1 : Split the data block.

            Case 2.2.1 : If the parent index block has space, insert the new key.
            Case 2.2.2 : If the parent index block is full
                Step 1 : Split the index block.
                Step 2 : Repeat until a parent index block with space is found or the root is reached.

                **Special Case:** If the root is full:
                Step 1 : Split the root.
                Step 2 : Create a new root.
}
```

## Assumptions
1. Balanced Splitting: When splitting a block (data or index), records or keys are evenly distributed while maintaining order.
2. Duplicate Check Before Insertion: A record is inserted only if it does not already exist in the tree.
3. Consistent First Data Block: The first data block is always ID = 1, storing the smallest values, simplifying traversal and debugging.

## Project Highlights
* Fully implemented B+ Tree with structured indexing.
* Efficient insertion & splitting methods for large datasets.
* Successfully tested with 10,000+ records and IDs up to 1,000,000.
* Memory-leak free implementation verified with Valgrind.

  ## Makefile
  ### Compile & Run
  #### First Implementation
  ```c
make bplus1
./build/bplus_main
  ```
#### Second Implementation
  ```c
make bplus2
./build/bplus_main
  ```
#### Clean Build Files
```c
make clean1  
make clean2  
```
### Important
Always run `make clean` before compiling to ensure proper compilation.
