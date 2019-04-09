#include <unistd.h>
#include <malloc.h>
#include <stdlib.h>
#include <memory.h>

#include "fat.h"

//creates vD
int createVirtualDisk(size diskSize)
{
    //try to create file
    size blocksAmount = howManyBlocks(diskSize);

    //our disk is: [super_block: size = sizeof(SuperBlocks)][fat: size = sizeof(int) * blocksAmount][rd: size = sizeof(dentry) * MAX_FILES_AMOUNT][BLOCK_SIZE * blocksAmount]
    size virtualDiskSize = sizeof(SuperBlock) + sizeof(int) + sizeof(int) * blocksAmount + sizeof(Dentry) * MAX_FILES_AMOUNT + BLOCK_SIZE * blocksAmount;

    FILE *vDisk = fopen("MY_FAT" , "wb");

    if (!vDisk)
    {
        printf("Error(fopen). Cannot create VirtualDisk. \n");

        return -1;
    }

    //"truncate - shrink or extend the size of a file to the specified size"
    truncate("MY_FAT", virtualDiskSize);

    //we have created our Virtual Disk, now set SuperBlock paramets


    superBlock.fatOffset = sizeof(SuperBlock); // because of our file structure ^^
    superBlock.rdOffset = superBlock.fatOffset + sizeof(int) * blocksAmount;
    superBlock.dataOffset = superBlock.rdOffset + sizeof(Dentry) * MAX_FILES_AMOUNT;

    superBlock.totalSize = virtualDiskSize;

    superBlock.allBlocks = blocksAmount;
    superBlock.freeBlocks = blocksAmount; //now all of them are free

    superBlock.allDentries = MAX_FILES_AMOUNT;
    superBlock.freeDentries = MAX_FILES_AMOUNT; //now all of them are free

    //
    FAT = malloc(sizeof(int) * superBlock.allBlocks); //

    if (!FAT)
    {
        printf("Error(malloc). Cannot create VirtualDisk. \n");

        return -2;
    }

    //all right - set default values in fat and rd
    for (size i = 0 ; i < superBlock.allBlocks ; ++i)
        FAT[i] = -1; //means that block is free


    for (size i = 0 ; i < superBlock.allDentries ; ++i)
        rootDirectory[i].valid = false;

    printf("VirtualDisk creating completed. \n");
    getFreeBlockIndex();
    return 0;
}

//delete vD
int deleteVirtualDisk()
{
    if (unlink("MY_FAT") == -1)
    {
        printf("Error(unlink). Cannot delete file system. \n");

        return -1;
    }

    printf("Virtual Disk deleting completed. \n");

    return 0;
}

//copy file to vD
int copyFileToVirtualDisk(char* fileName)
{
    //check if opertion is possible

    //is right name?
    if (strlen(fileName) > MAX_NAME_LENGTH)
    {
        printf("Error(file name too long). Cannot copy file to Virtual Disk. \n");

        return -1;
    }

    //is this filename actually in use?
    if (findFileName(fileName) >= 0)
    {
        printf("Error(file name '%s' is already in use). Cannot copy file to Virtual Disk. \n" , fileName);

        return -2;
    }

    //is any freeDentry in vD?
    if (superBlock.freeDentries == 0 )
    {
        printf("Error(no free dentries). Cannot copy file to Virtual Disk. \n");

        return -3;
    }
    //now open files - origin file and destination (vD)

    FILE *originFile , *vDisk;

    vDisk = fopen("MY_FAT" , "r+b"); //read and write
    originFile = fopen(fileName , "rb"); //read

    if (!originFile)
    {
        printf("Error(fopen). Cannot open origin file. \n");

        return -4;
    }

    if (!vDisk)
    {
        printf("Error(fopen). Cannot open virtual disk. \n");
        //but opened origin file before- we have to close it.
        fclose(originFile);

        return -5;
    }

    //we opened both files, we can check origin file size
    //int fseek(FILE *file, long offset, int mode);
    fseek(originFile , 0 , SEEK_END); //end - 0 = end
    size fileSize = ftell(originFile);
    //rewind - 'return' to file begin ( ==  fseek(file, 0, 0).)
    rewind(originFile);

    //is enough free space on our vd?
    if (superBlock.freeBlocks * BLOCK_SIZE < fileSize)
    {
        printf("Error(not enough free space). There is not enough place on disk. \n");
        //close files
        fclose(originFile);
        fclose(vDisk);

        return -6;
    }

    //there all conditions are met - ''just'' copy

    size blockAmount = howManyBlocks(fileSize);

    //looking for non use dentry in rd
    Dentry thisDentry;
    //set parametrs
    thisDentry.firstBlock = getFreeBlockIndex();
    if (thisDentry.firstBlock < 0)
    {
        printf("Error. Cannot find free block(s) in memory. \n");
        fclose(originFile);
        fclose(vDisk);

        return -7;
    }

    thisDentry.valid = true;
    thisDentry.size = fileSize;
    strcpy(thisDentry.name, fileName);
    thisDentry.creationDate = time(NULL); //number of seconds since the Epoch
    thisDentry.modificationDate = thisDentry.creationDate;

    size tempOffset;
    size dataToCopy = fileSize;
    size tempFatIndex = thisDentry.firstBlock;


    //read data from origin file:
    //size_t fread ( void * ptr, size_t size, size_t count, FILE * stream ) size - in bytes, of each element to be read (in our case  char size (1),
    //count - Number of elements, each one with a size of size bytes (in oc blocksAmount, ptr - destination (block in our disk), stream - origin file
    //we are reading chars, to char table, char table size = BLOCK_SIZE
    //check fread - it return number of elements which was correctly read

    char singleDataBlock[BLOCK_SIZE];


    if (dataToCopy <= 0)
    {
        printf("Error. File size = 0. \n");
        fclose(originFile);
        fclose(vDisk);

        return -7;
    }

    size nextFatIndex = -2;
    FAT[tempFatIndex] = -2;

    while (1)
    {

        FAT[tempFatIndex] = -2;
        nextFatIndex = getFreeBlockIndex();
        FAT[tempFatIndex] = nextFatIndex;

        tempOffset = superBlock.dataOffset + tempFatIndex * BLOCK_SIZE;
        size readElem = fread(&singleDataBlock , 1 , BLOCK_SIZE , originFile); // 1- sizeof char


        if (readElem != BLOCK_SIZE && (dataToCopy >= BLOCK_SIZE || readElem != fileSize%BLOCK_SIZE))
        {

            printf("Error(fread). Cannot copy file. \n");
            //delete data
            deleteFromFat(thisDentry.firstBlock);
            //close files
            fclose(originFile);
            fclose(vDisk);

            return -8;
        }

        dataToCopy -= readElem;

        tempFatIndex = nextFatIndex;

        fseek(vDisk, tempOffset, SEEK_SET); //int fseek(FILE *file, long offset, int mode); SEEK_SET - It moves file pointer position to the beginning of the file.

        if (fwrite(&singleDataBlock , 1 , readElem , vDisk) != readElem)
        {
            printf("Error(fwrite). Cannot copy file. \n");
            //delete data
            deleteFromFat(thisDentry.firstBlock);
            //close files
            fclose(originFile);
            fclose(vDisk);

            return -9;
        }

        FAT[tempFatIndex] = -2; //means end of file, temporary, because of getting functions

        if (dataToCopy <= 0)
            break;

        nextFatIndex = getFreeBlockIndex();
        FAT[tempFatIndex] = nextFatIndex;
    }



    rootDirectory[getFreeDentryIndex()] = thisDentry;
    --(superBlock.freeDentries);
    superBlock.freeBlocks -= blockAmount;

    fclose(vDisk);
    fclose(originFile);

    printf("Copying file '%s' to VirtualDisk completed. \n" ,  fileName);

    return 0;
}

//copy file from vD
int copyFileFromVirtualDisk(char* fileName)
{
    FILE *destinationFile , *vDisk;

    //check if file with fileName exist in vDisk

    size toCopyDentryIndex = findFileName(fileName);
    if (toCopyDentryIndex < 0) //we didnt find file in our system - function return -3 value
    {
        printf("Error. A file does not exist. \n");

        return -1;
    }


    destinationFile = fopen(fileName, "wb"); //we need to create new file
    vDisk = fopen("MY_FAT", "rb"); //we only want to read from our disk

    if (!destinationFile)
    {
        printf("Error(fopen). Cannot open origin file. \n");
        //but opened vDisk, close it.
        return -2;
    }

    if (!vDisk)
    {
        printf("Error(fopen). Cannot open virtual disk. \n");
        //but opened origin file before- we have to close it.
        fclose(destinationFile);

        return -3;
    }

    //there all conditions are met - ''just'' copy


   //looking for dentry "where" is this file
    Dentry thisDentry = rootDirectory[toCopyDentryIndex];

    size tempOffset;

    size dataToCopy = thisDentry.size;
    size tempFatIndex = thisDentry.firstBlock;


    //read data from origin file:
    //size fread ( void * ptr, size_t size, size_t count, FILE * stream ) size - in bytes, of each element to be read (in our case  char size (1),
    //count - Number of elements, each one with a size of size bytes (in oc blocksAmount, ptr - destination (block in our disk), stream - origin file
    //we are reading chars, to char table, char table size = BLOCK_SIZE
    //check fread - it return number of elements which was correctly read

    char singleDataBlock[BLOCK_SIZE];

    if (dataToCopy <= 0)
    {
        printf("Error. File size <= 0. \n");
        fclose(destinationFile);
        fclose(vDisk);

        return -4;
    }

    size nextFatIndex = FAT[tempFatIndex];

    while (dataToCopy > 0 || tempFatIndex < 0)
    {

        tempOffset = superBlock.dataOffset + tempFatIndex * BLOCK_SIZE;
        fseek(vDisk , tempOffset , SEEK_SET); //from beg + offset


        size readElem;

        if (dataToCopy >= BLOCK_SIZE)
            readElem = fread(&singleDataBlock , 1 , BLOCK_SIZE , vDisk); // 1 =  sizeof char
        else
            readElem = fread(&singleDataBlock , 1 , dataToCopy , vDisk); // 1 =  sizeof char


        if (readElem != BLOCK_SIZE && (dataToCopy >= BLOCK_SIZE || readElem != dataToCopy))
        {

            printf("Error(fread). Cannot copy file. \n");

            //close files
            fclose(destinationFile);
            fclose(vDisk);

            return -5;
        }

        dataToCopy -= readElem;

        tempFatIndex = FAT[tempFatIndex];

        //fseek(vDisk , tempOffset , SEEK_SET); //int fseek(FILE *file, long offset, int mode)

        if (fwrite(&singleDataBlock , 1 , readElem , destinationFile) != readElem)
        {
            printf("Error(fwrite). Cannot copy file. \n");

            //close files
            fclose(destinationFile);
            fclose(vDisk);

            return -6;
        }
    }

    fclose(vDisk);
    fclose(destinationFile);

    printf("Copying file '%s' from VirtualDisk completed. \n" ,  fileName);

    return 0;
}

//delete file from vD
int deleteFileFromVirtualDisk(char* fileName)
{
    FILE *vDisk;

    //check if file with fileName exist in vDisk
    if (findFileName(fileName) < 0) //we didnt find file in our system - function return -3 value
    {
        printf("Error. A file does not exist. \n");

        return -1;
    }

    //we know that file exixst , try to open it
    vDisk = fopen("MY_FAT", "rb");

    if (!vDisk)
    {
        printf("Error. Failed to open VirtualDisk\n");

        return -2;
    }

    //we open file- delete it from rd table and fat table
    size toDelDentry;

    //find file in rd
    for (size i = 0 ; i < superBlock.allDentries ; ++i)
    {
        if (rootDirectory[i].valid && (strcmp(rootDirectory[i].name, fileName) == 0))
        {
            toDelDentry = i;

            break;
        }
    }

    deleteFromFat(rootDirectory[toDelDentry].firstBlock);
    rootDirectory[toDelDentry].valid = false;

    superBlock.freeBlocks += howManyBlocks(rootDirectory[toDelDentry].size);
    ++(superBlock.freeDentries);

    fclose(vDisk);

    printf("Deleting file '%s' from VirtualDisk completed. \n" ,  fileName);

    return 0;
}

//display vD directory
int displayVirtualDiskDirectory()
{
    printf(" Size : \t %d \n Free space : \t %d \n Number of blocks : \t %d \n Number of free blocks : \t %d  \n Number of directory entries : \t %d \n Number of free directory entries : \t %d \n"
            , superBlock.totalSize , superBlock.freeBlocks * BLOCK_SIZE , superBlock.allBlocks , superBlock.freeBlocks , superBlock.allDentries , superBlock.freeDentries);
}

//display vD map
int displayVirtualDiskMap()
{
    for (size i = 0 ; i < superBlock.allDentries ; ++i)
        if (rootDirectory[i].valid == true)
            printf("%d) File name : %s| Size : %d | C_time (number of seconds since the Epoch): %ld  | M_time (number of seconds since the Epoch): %ld | \n" , i+1 , rootDirectory[i].name , rootDirectory[i].size ,
                    rootDirectory[i].creationDate , rootDirectory[i].modificationDate);
}

int deleteFromFat(size firstBlockIndex)
{
    size tempIndex = firstBlockIndex;

    do
    {
        size nextIndex = FAT[tempIndex];
        FAT[tempIndex] = -1; //means free
        tempIndex = nextIndex;
    }  while (tempIndex != -2);

}


size getFreeBlockIndex()
{
    for (size i = 0 ; i < superBlock.allBlocks ; ++i)
    {
        if (FAT[i] == -1) //-1 means block is free
            return i; //return index
    }


    return -3; //means all blocks are in use
}
size getFreeDentryIndex()
{
    for (size i = 0 ; i < superBlock.allDentries ; ++i)
    {
        if (rootDirectory[i].valid == false)
            return i; //return index
    }

    return -3; //means there is no empty dentries
}
//returns amount of blocks
size howManyBlocks(size spaceSize)
{
    if (spaceSize % BLOCK_SIZE == 0)
        return (size)spaceSize / BLOCK_SIZE;
    else
        return (size)spaceSize / BLOCK_SIZE + 1;
}


//updates VirtualDisk parametrs - fat, rd, sb
int updateVirtualDisk()
{
    FILE *vDisk = fopen("MY_FAT", "r+b");

    if (!vDisk)
    {
        printf("Error(fopen). Cannot open VirtualDisk. \n");

        return -1;
    }

    if (fwrite(&superBlock, sizeof(SuperBlock), 1, vDisk) != 1)
    {

        printf("Error. Cannot update SuperBlock. \n");

        fclose(vDisk);

        return -2;
    }

    if (fwrite(FAT, sizeof(int), superBlock.allBlocks, vDisk) != superBlock.allBlocks )
    {

        printf("Error. Cannot update FAT. \n");

        fclose(vDisk);

        return -3;
    }

    if (fwrite(rootDirectory, sizeof(Dentry), MAX_FILES_AMOUNT, vDisk) != MAX_FILES_AMOUNT)
    {

        printf("Error. Cannot update RootDirectory. \n");

        fclose(vDisk);

        return -4;
    }

    fclose(vDisk);
    printf("Virtual Disk updating completed.\n");

    return 0;
}

//opens VirtualDisk if it is possible
int openVirtualDisk()
{
    //open file
    FILE *vDisk = fopen("MY_FAT", "rb");

    if (!vDisk)
    {
        printf("Error(fopen). Cannot open VirtualDisk. \n");

        return -1;
    }

    //load SuperBlock, fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
    if (fread(&superBlock, sizeof(SuperBlock) , 1 , vDisk) != 1) //fread() and fwrite() return the number of items read or written
    {
        printf("Error(fread). Cannot read superBlock. \n");

        fclose(vDisk);

        return -2;
    }

    //alloc mem
    FAT = malloc(sizeof(int) * superBlock.allBlocks);

    if (!FAT)
    {
        printf("Error(malloc). Cannot allocate memory for FAT. \n");

        fclose(vDisk);

        return -3;
    }

    // load fat
    if (fread(FAT, sizeof(int) , superBlock.allBlocks , vDisk) != superBlock.allBlocks)
    {

        printf("Error(fread). Cannot read FAT. \n");

        free(FAT);

        fclose(vDisk);

        return -4;
    }

    //load rd
    if (fread(rootDirectory , sizeof(Dentry) , MAX_FILES_AMOUNT , vDisk) != MAX_FILES_AMOUNT)
    {

        printf("Error. Cannot read RootDirectory. \n");

        free(FAT);

        fclose(vDisk);

        return -5;
    }


    fclose(vDisk);

    printf("Virtual Disk opening completed. \n");

    return 0;
}

//free allocated memory before quit program
void closeVirtualDisk()
{
    if (FAT != NULL) //if we have sth to free...
        free(FAT);
}

//check if this file name exist in vDisk, return index or -1 if false
int findFileName(char* fileName)
{
    for (size i = 0 ; i < superBlock.allDentries ; ++i)
    {
        if (rootDirectory[i].valid)
        {
            if ((strcmp(rootDirectory[i].name , fileName)) == 0)
                return i;
        }
    }

    return -1;
}
int main(int argc, char* argv[])
{

    if (!(argc == 2 || argc == 3))
    {
        printf("Invalid number of arguments. Type 'h' to display help. \n");
        return -1;
    }

    if (argv[1][0] == 'h')
    {
        printf("First argument possible values: \n"
               "'c' 'diskSize' - create virtual disk, diskSize > 0 \n"
             "'d' - delete virtual disk \n"
             "'e' - copy file from virtual disk \n"
             "'i' - copy file to virtual disk \n"
             "'x' - delete file from virtual disk \n"
             "'s' - display virtual disk directory \n"
             "'m' - display virtual disk map \n");


        return 0;
    }

    else if (argv[1][0] == 'c' && argc == 3)
    {
      if (atoi(argv[2]) <= 0)
        {
            printf("Invalid value of second argument. Type 'h' to display help. \n");

            return -1;

        }
        createVirtualDisk((size)atoi(argv[2]));
        updateVirtualDisk(); //we may have changed fat, rd, sb parametrs - so udpate them
        closeVirtualDisk(); //free memory before ending program

        return 0;
    }

    else if (argv[1][0] == 'd' && argc == 2)
    {
        deleteVirtualDisk();

        return 0;
    }

    else if (argv[1][0] == 'e' && argc == 3)
    {
        if(openVirtualDisk() == 0)
            copyFileFromVirtualDisk(argv[2]);
        closeVirtualDisk();

        return 0;
    }

    else if (argv[1][0] == 'i' && argc == 3)
    {
        if(openVirtualDisk() == 0)
            copyFileToVirtualDisk(argv[2]);
        updateVirtualDisk();
        closeVirtualDisk();

        return 0;
    }

    else if (argv[1][0] == 'x' && argc == 3)
    {
        if(openVirtualDisk() == 0)
            deleteFileFromVirtualDisk(argv[2]);
        updateVirtualDisk();
        closeVirtualDisk();


        return 0;
    }


    else if (argv[1][0] == 's' && argc == 2)
    {
        if(openVirtualDisk() == 0)
            displayVirtualDiskDirectory();
        closeVirtualDisk();

        return 0;
    }

    else if (argv[1][0] == 'm' && argc == 2)
    {
        if(openVirtualDisk() == 0)
            displayVirtualDiskMap();
        closeVirtualDisk();

        return 0;
    }

    else
    {
        printf("Invalid value of first argument. Type 'h' to display help. \n");

        return -2;
    }

}
