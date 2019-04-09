#ifndef FAT_H_INCLUDED
#define FAT_H_INCLUDED

#include <glob.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <stdbool.h>
#include <time.h>

#define MAX_NAME_LENGTH 30
#define MAX_FILES_AMOUNT 100
#define BLOCK_SIZE 4096


typedef uint32_t size;

typedef struct
{
    size fatOffset;
    size rdOffset;
    size dataOffset;

    size totalSize;

    size allBlocks;
    size freeBlocks;

    size allDentries;
    size freeDentries;

} SuperBlock;

typedef struct
{
    char name[MAX_NAME_LENGTH + 1];
    size size;
    time_t creationDate, modificationDate;
    size firstBlock;
    bool valid;
} Dentry;

SuperBlock superBlock;
Dentry rootDirectory[MAX_FILES_AMOUNT];
int* FAT; // value -1 means free block, value -2 means last block, other values means next block index

//obligatory functions:
int createVirtualDisk(size diskSize);//++
int deleteVirtualDisk();//++

int copyFileToVirtualDisk(char* fileName);//+

int copyFileFromVirtualDisk(char* fileName);//+
int deleteFileFromVirtualDisk(char* fileName);//+

int displayVirtualDiskDirectory();//+
int displayVirtualDiskMap();//+

//display x 2...

//additional functions/helpers:
size howManyBlocks(size spaceSize);//++

int updateVirtualDisk(); //+
int updateFAT();//+
int updateRD();//+
int updateSB();//+

int openVirtualDisk();//++
void closeVirtualDisk();//++

size getFreeBlockIndex();//++
size getFreeDentryIndex();//++

int deleteFromFat(size firstBlockIndex);//++

int findFileName(char* fileName);//++ return -3 when doesnt exist
#endif // FAT_H_INCLUDED
