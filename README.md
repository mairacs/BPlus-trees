# B+ trees in the course of Database Management Systems Implementation

## Team Information
Maira Papadopoulou
Nektarios Pavlakos-Tsimpourakis

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





