// THIS MAIN USES SRAND() WITH time.h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "bp_file.h"
#include "bp_datanode.h"
#include "bp_indexnode.h"

#define RECORDS_NUM 10000 // you can change it if you want
#define FILE_NAME "data.db"

#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }

void insertEntries();
void findEntries();

int main() {
  
  srand(time(NULL));

  insertEntries();
  findEntries();

  return 0;
}

void insertEntries(){
  BF_Init(LRU);
  BP_CreateFile(FILE_NAME);

  int file_desc;
  BPLUS_INFO* info = BP_OpenFile(FILE_NAME, &file_desc);
  if (info == NULL) {
      fprintf(stderr, "Error opening file: %s\n", FILE_NAME);
      exit(EXIT_FAILURE);
  }

  Record record;
  for (int i = 0; i < RECORDS_NUM; i++)
  {
    record = randomRecord();
    BP_InsertEntry(file_desc,info, record);
  }

  printf("All the Datanodes:\n");
  BP_PrintDatanodes(file_desc, info);
  BP_CloseFile(file_desc,info);
  BF_Close();
}

void findEntries(){
  int file_desc;
  BPLUS_INFO* info;

  BF_Init(LRU);
  info=BP_OpenFile(FILE_NAME, &file_desc);
  if (info == NULL) {
    fprintf(stderr, "Error opening file: %s\n", FILE_NAME);
    exit(EXIT_FAILURE);
  }

  Record tmpRec;
  Record* result=&tmpRec;
  
  int id=0; 
  printf("Searching for record with ID: %d\n", id);

  BP_GetEntry( file_desc,info, id,&result);
  if(result!=NULL) {
    printRecord(*result);
  }
  else {
    printf("Record with ID %d not found.\n", id);
  }
  printf("\n");

  id=rand() % 750; 
  printf("Searching for record with ID: %d\n", id);

  BP_GetEntry( file_desc,info, id,&result);
  if(result!=NULL) {
    printRecord(*result);
  }
  else {
    printf("Record with ID %d not found.\n", id);
  }
  printf("\n");

  id=1000000; 
  printf("Searching for record with ID: %d\n", id);

  BP_GetEntry( file_desc,info, id,&result);
  if(result!=NULL) {
    printRecord(*result);
  }
  else {
    printf("Record with ID %d not found.\n", id);
  }
  printf("\n");

  BP_CloseFile(file_desc,info);
  BF_Close();
}
