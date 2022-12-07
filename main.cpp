#include <iostream>
#include <fstream>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>

class myFileSystem {

private:
    int position;

public:

    void theFileSystem(char diskName[16]) { //const string& diskName
        // open the file with the above name, this file will act as the "disk" for your file system
        int fd = open(diskName, O_RDWR);

        if (fd < 0) {
            std::cout << "[ERROR] File '" << diskName << "' could not be opened." << std::endl;
            exit(1);
        } else {
            std::cout << "--------------------------------------------------" << std::endl;
            std::cout << "[SUCCESS] File '" << diskName << "' was opened." << std::endl;
        }
        position = fd; // starting position
    }

    void initSuperBlock() const {
        // Set file pointer to the start of disk
        lseek(position, 0, SEEK_SET);

        // Using a buffer to set Free Block List (FBL) to 1 since its always being used
        char dummyBufferFBLFull[1];
        dummyBufferFBLFull[0] = '1';
        write(position, dummyBufferFBLFull, 1);

        // Setting all other bytes, which are inodes, to 0 (unused) initially
        char dummyBufferEmpty[1];
        dummyBufferEmpty[0] = '0';
        char dummyBufferInodes[1];
        dummyBufferInodes[0] = '0';
        for (int i = 1; i < 1024; i++) {
            if (i < 128) {
                write(position, dummyBufferEmpty, 1);
            } else {
                write(position, dummyBufferInodes, 1);
            }
        }
    }

    void create(char filename[16], int filesize) { //create a file with this name and this size
        std::cout << "Creating file with name '" << filename << "' and size " << filesize << std::endl;
        //
        /* Step 1: check to see if we have sufficient free space on disk by reading in the free block list.
            To do this:
             1. move the file pointer to the start of the disk file.
             2. Read the first 128 bytes (the free/in-use block information)
             3. Scan the list to make sure you have sufficient free blocks to allocate a new file of this size */

        // left and right pointers of sliding window
        int left = 0, right = 0;
        char blockByte;
        lseek(position, 0, SEEK_SET);

        // Read the first 128 bytes and search for a allocation space that can fit the filesize
        for (int i = 0; i < 128; i++) {
            read(position, &blockByte, 1);
            if (blockByte == '0') { // free memory
                right++;
            } else {
                left = i + 1;
                right = i + 1;
            }
            if (right - left == filesize) {
                std::cout << "Found sufficient free space on the disk." << std::endl;
                break;
            }
            lseek(position, 1, SEEK_CUR);
        }

        /* Step 2: we look for a free inode on disk
           - Read in an inode
           - check the "used" field to see if it is free
           - If not, repeat the above two steps until you find a free inode
           - Set the "used" field to 1
           - Copy the filename to the "name" field
           - Copy the file size (in units of blocks) to the "size" field */

        // First Inode Param Positions: name at 128 | size at 143 | block p's at 175 | used at 179 (56B Total)
        const int firstInode = 128;
        int curInodePos = firstInode;

        for (int i = 0; i <= 16; i++) { // The file system supports a max of 16 files
            // read in name
            char name[16]; // the file name must be unique and can be no longer than 15 char's (+ \0 byte)
            lseek(position, curInodePos, SEEK_SET);
            read(position, &name, 15);
            name[15] = '\0';

            // read in size
            char size[5];
            read(position, &size, 4);
            size[4] = '\0';

            // read in blockPointer
            char blockPointer[32];
            read(position, &blockPointer, 32);
            blockPointer[32] = '\0';

            // read in used
            char used[4];
            read(position, &used, 4);
            used[4] = '\0';
            int us = atoi(used);

            /* ----------- ADDING AN INODE ----------- */
            if (us == 0) { // unused inode found

                // writing name
                lseek(position, curInodePos, SEEK_SET);
                write(position, filename, strlen(filename));

                // writing size
                snprintf(size, 4, "%d", filesize);
                lseek(position, curInodePos + 16, SEEK_SET);
                write(position, size, strlen(size));

                // setting used bit
                const char isUsed[5] = {'1', '0', '0', '|', '\0'};
                lseek(position, curInodePos + 52, SEEK_SET);
                write(position, isUsed, strlen(isUsed));
                break;
            }
            curInodePos += 56; // move on to next inode
        }

        /* Step 3: Allocate data blocks to the file
            - for(i=0;i<size;i++)
            - Scan the block list that you read in Step 1 for a free block
            - Once you find a free block, mark it as in-use (Set it to 1)
            - Set the blockPointer[i] field in the inode to this block number */

        for (int i = 0; i < filesize; i++) {
            lseek(position, 0, SEEK_SET); // set post to head of fbl
            char blockStatus;
            for (int blockNumber = 0; blockNumber < 128; blockNumber++) { // scan through all of fbl
                read(position, &blockStatus, 1); // read in byte, which moves position +1
                if (blockStatus == '0') { // found free memory
                    blockStatus = '1';
                    char writeBlockStatus[4];
                    snprintf(writeBlockStatus, 4, "%d", filesize);

                    lseek(position, -1,
                          SEEK_CUR); // last byte read was 0, so go back one, since write moves pos  forward 1
                    char blockN[5];
                    snprintf(blockN, 4, "%d", blockNumber);
//                    std::cout << "BLOCK NUMBER: " << blockN << std::endl;

                    write(position, &blockStatus, 1); // now we are past the 1 we wrote

                    if (i == 0) {
                        lseek(position, curInodePos + 20,
                              SEEK_SET); // 16 (name) + 4 (size) = 20 at start of current inode
                    } else {
                        lseek(position, curInodePos + 20 + (i * 4),
                              SEEK_SET); // 16 (name) + 4 (size) = 20 at start of current inode
                    }

                    if (strlen(blockN) == 1) {
                        blockN[1] = '-';
                        blockN[2] = '-';
                        blockN[3] = '-';
                        blockN[4] = '\0';
                    }
                    if (strlen(blockN) == 2) {
                        blockN[2] = '-';
                        blockN[3] = '-';
                        blockN[4] = '\0';
                    }
                    if (strlen(blockN) == 3) {
                        blockN[3] = '-';
                        blockN[4] = '\0';
                    }

                    write(position, blockN, 4);
                    break;
                }
            }
        }

        /* Step 4: Write out the inode and free block list to disk
            - Move the file pointer to the start of the file
            - Write out the 128 byte free block list
            - Move the file pointer to the position on disk where this inode was stored
            - Write out the inode */

        std::cout << "-------- Free Block List --------" << std::endl;
        lseek(position, 0, SEEK_SET);
        for (int i = 0; i < 128; i++) {
            char fblValue;
            read(position, &fblValue, 1);
            std::cout << fblValue << "";
            if (i == 31 || i == 63 || i == 95 || i == 127) {
                std::cout << "\n";
            }
        }

        // read in name that I wrote and clean empty indexes
        lseek(position, curInodePos, SEEK_SET);
        char name[16];
        char cleanName[strlen(filename)];
        read(position, &name, 15);
        name[15] = '\0';
        memcpy(&cleanName, name, strlen(filename));

        // read in the size
        lseek(position, 1, SEEK_CUR);
        char readSize[5];
        read(position, &readSize, 4);
        readSize[4] = '\0';

        // read in block pointers
        char blockPointer[33];
        read(position, &blockPointer, 32);
        blockPointer[32] = '\0';

        // read in used bit (0 = unused, 1 = used)
        char used[5];
        read(position, &used, 4);
        used[4] = '\0';

        std::cout << "--------------------- inode ----------------------\n" << "Name: " << cleanName << "\nSize: "
                  << readSize[0] << "KB" << "\n";
        std::cout << "Block Pointers: [ ";

        // 1---3---4---00000000
        // loop 8 times (max size)
            // if i == 0 && buf[i] != '0'
                // loop 4 times (blockPointer size)
            // if i > 0 && buf[i*4] != '0'
                // loop 4 times (blockPointer size)

        for (int i =0 ; i < 32; i+=4){
            if (blockPointer[i] != '0'){
                std::cout << blockPointer[i];
                if (blockPointer[i+1] != '-'){
                    std::cout << blockPointer[i+1];
                    if(blockPointer[i+2] != '-'){
                        std::cout << blockPointer[i+2];
                    }
                }
                std::cout << " ";
            }
        }
        std::cout << "]\nUsed: " << used[0] << std::endl;
    }

    void ls() const {
        /* List names of all files on disk
           - Step 1: read in each inode and print!
           - Move file pointer to the position of the 1st inode (129th byte)
           - for(i=0;i<16;i++)
           - Read in a inode
           - If the inode is in-use
           - print the "name" and "size" fields from the inode */
        int curInodePos = 128;
        for (int i = 0; i <= 16; i++) { // The file system supports a max of 16 files
            // read in name
            char name[16]; // the file name must be unique and can be no longer than 15 char's (+ \0 byte)
            lseek(position, curInodePos, SEEK_SET);
            read(position, &name, 15);
            name[15] = '\0';
//            std::cout << "Name: '" << name << "'" << std::endl;
            // TODO why is it that when uncomment the above line, ls works perfect
            int filenameLength = 0;
            for (char j: name) {
                if (j != '0') {
                    filenameLength++;
                }
            }

            // read in size
            lseek(position, 1, SEEK_CUR);
            char size[5];
            read(position, &size, 4);
            size[4] = '\0';

            // read in blockPointer
            char blockPointer[32];
            read(position, &blockPointer, 32);
            blockPointer[31] = '\0';

            // read in used
            char used[2];
            read(position, &used, 1);
            used[1] = '\0';
            int us = atoi(used);

            /* ----------- ADDING AN INODE ----------- */
            if (us == 1) { // found a used inode
                lseek(position, curInodePos, SEEK_SET);
                char readInFileName[filenameLength + 1]; // a.out has 5 chars, we need [0->5] = 6 chars
                for (unsigned x = 0; x < strlen(readInFileName); x++) {
                    readInFileName[x] = ' ';
                }
//                std::cout << "READINFILENAMEBEFORE: " << readInFileName << std::endl;
                read(position, &readInFileName, filenameLength - 1); //read in 5 letters a.out
                readInFileName[filenameLength] = '\0';// add a null byte to the 6th position
                std::cout << readInFileName << " [" << size[0] << "KB]\t";
                if (i == 3 || i == 7 || i == 11 || i == 15) {
                    std::cout << "\n";
                }
                curInodePos += 56; // move on to next inode
            }
        }
        std::cout << "\n";
    }

    int readBlockFromFile(char filename[16], int blockNum, char buf[1024]) {
        // read this block from this file
        std::cout << "[READ] Reading block #" << blockNum << " from file '" << filename << "'" << std::endl;
        /*  Step 1: locate the inode for this file
            Move file pointer to the position of the 1st inode (129th byte)
            Read in a inode
            If the inode is in use, compare the "name" field with the above file
            IF the file names don't match, repeat*/
        int curInodePos = 128;
        for (int i = 0; i <= 16; i++) { // The file system supports a max of 16 files
            // read in name
            char name[16]; // the file name must be unique and can be no longer than 15 char's (+ \0 byte)
            lseek(position, curInodePos, SEEK_SET);
            read(position, &name, 15);
            name[15] = '\0';

            int filenameLength = 0;
            for (char j: name) {
                if (j != '0') {
                    filenameLength++;
                }
            }

            // read in size
            lseek(position, 1, SEEK_CUR);
            char size[5];
            read(position, &size, 4);
            size[4] = '\0';
            char singleSize[1];
            singleSize[0] = size[0];
            singleSize[1] = '\0';
            int sz = atoi(singleSize);

            // read in blockPointer
            char blockPointer[32];
            read(position, &blockPointer, 32);
            blockPointer[31] = '\0';

            // read in used
            char used[2];
            read(position, &used, 1);
            used[1] = '\0';
            int us = atoi(used);

            /* ----------- ADDING AN INODE ----------- */
            if (us == 1) { // found a used inode
                lseek(position, curInodePos, SEEK_SET); // set fp to start of current inode
                char readInFileName[filenameLength + 1]; // a.out has 5 chars, we need [0->5] = 6 chars
                for (unsigned int x = 0; x < strlen(readInFileName); x++) {
                    readInFileName[x] = ' ';
                }
                read(position, &readInFileName, filenameLength - 1); //read in 5 letters a.out
                readInFileName[filenameLength - 1] = '\0';// add a null byte to the 6th position

                int stringEquality = strcmp(readInFileName, filename);
                if (stringEquality == 0) {
                    std::cout << "---------- [FILE FOUND] ----------\n"
                                "Name: " << filename << ", Size: " << sz << "\n----------------------------------" << std::endl;
                    break;
                } else {
                    curInodePos += 56; // move on to next inode
                    continue;
                }
            }
            /*  Step 2: Read in the specified block
                Check that blockNum < inode.size, else flag an error
                Get the disk address of the specified block
                That is, addr = inode.blockPointer[blockNum]
                move the file pointer to the block location (i.e., to byte # addr*1024 in the file)*/
            if (blockNum > sz){
                std::cout << "[ERR] Cannot read block #" << blockNum << " since file '" << name << "' has size of " << sz << "KB." << std::endl;
                exit(1);
            }
            // Read in the block! => Read in 1024 bytes from this location into the buffer "buf"
            if (blockNum == 0) {
                lseek(position, curInodePos + 20, SEEK_SET); // positioned at inodes block pointer section
            } else {
                lseek(position, curInodePos + 20 + (blockNum * 4), SEEK_SET); // positioned at inodes block pointer section
            }
            read(position, buf, 1024);
        }
        std::cout << "Read at block #" << blockNum << " completed!\n";
        for (int i = 1; i < 1025; i++){
            std::cout << buf[i-1];
            if( i % 32 == 0 && i != 0 ){
                std::cout << "\n";
            }
        }
        return 1;
    }

    int writeBlockToFile(char filename[16], int blockNum, char buf[1024]) {
        // write this block to this file
        std::cout << "[WRITE] Writing to block #" << blockNum << " in file '" << filename << "'" << std::endl;
        /* Step 1: locate the inode for this file
            Move file pointer to the position of the 1st inode (129th byte)
            Read in a inode
            If the inode is in use, compare the "name" field with the above file
            If the file names don't match, repeat*/
        int curInodePos = 128;
        int foundInode;
        int foundSize;
        int foundFlag = 0;
        for (unsigned int i = 0; i <= 16; i++) { // The file system supports a max of 16 files
            // read in name
            char name[16]; // the file name must be unique and can be no longer than 15 char's (+ \0 byte)
            lseek(position, curInodePos, SEEK_SET);
            read(position, &name, 15);
            name[15] = '\0';

            int filenameLength = 0;
            for (char j: name) {
                if (j != '0') {
                    filenameLength++;
                }
            }

            // read in size
            lseek(position, 1, SEEK_CUR);
            char size[5];
            read(position, &size, 4);
            size[4] = '\0';
            char singleSize[1];
            singleSize[0] = size[0];
            singleSize[1] = '\0';
            int sz = atoi(singleSize);

            // read in blockPointer
            char blockPointer[32];
            read(position, &blockPointer, 32);
            blockPointer[31] = '\0';

            // read in used
            char used[2];
            read(position, &used, 1);
            used[1] = '\0';
            int us = atoi(used);

            /* ----------- ADDING AN INODE ----------- */
            if (us == 1) { // found a used inode
                lseek(position, curInodePos, SEEK_SET); // set fp to start of current inode
                char readInFileName[filenameLength + 1]; // a.out has 5 chars, we need [0->5] = 6 chars
                for (unsigned int x = 0; x < strlen(readInFileName); x++) {
                    readInFileName[x] = ' ';
                }

                read(position, &readInFileName, filenameLength - 1); //read in 5 letters a.out
                readInFileName[filenameLength - 1] = '\0';// add a null byte to the 6th position

                int stringEquality = strcmp(readInFileName, filename);
                if (stringEquality == 0) {
                    foundFlag = 1;
                    // we found the file we need to write to, save this position
                    foundInode = curInodePos;
                    foundSize = sz;
                    std::cout << "{INODE FOUND} Name: " << filename << ", Pos: " << foundInode << ", Size: "
                              << foundSize << std::endl;
                    break;
                } else {
                    curInodePos += 56; // move on to next inode
                    continue;
                }
            }
        }

        if (foundFlag != 1) {
            std::cout << "[ERR] No file found with name '" << filename << "'" << std::endl;
            return 1;
        }

        /* Step 2: Write to the specified block
            Check that blockNum < inode.size, else flag an error
            Get the disk address of the specified block
            That is, addr = inode.blockPointer[blockNum]
            move the file pointer to the block location (i.e., byte # addr*1024) */
        if (blockNum > foundSize) {
            std::cout << "[ERR] Out of bounds: Cannot write to block #" << blockNum << " since size of file = "
                      << foundSize << "KB." << std::endl;
            exit(1);
        }

        if (blockNum == 0) {
            lseek(position, foundInode + 20, SEEK_SET); // positioned at inodes block pointer section
        } else {
            lseek(position, foundInode + 20 + (blockNum * 4),
                  SEEK_SET); // positioned at inodes block pointer section
        }

        char dataBlockPos[5];
        read(position, &dataBlockPos, 4);
        dataBlockPos[4] = '\0';
        int posToWriteDataBlockTo = atoi(dataBlockPos) * 1024;
        // Write the block! => Write 1024 bytes from the buffer "buff" to this location
        lseek(position, posToWriteDataBlockTo, SEEK_SET);
        write(position, buf, 1024);
        return 1;
    }

    /*    public int detete(char name[16]) {
         Delete the file with this name

         Step 1: Locate the inode for this file
         Move the file pointer to the 1st inode (129th byte)
         Read in a inode
         If the iinode is free, repeat above step.
         If the iinode is in use, check if the "name" field in the
         inode matches the file we want to delete. IF not, read the next
          inode and repeat

         Step 2: free blocks of the file being deleted
         Read in the 128 byte free block list (move file pointer to start of the disk and read in 128 bytes)
         Free each block listed in the blockPointer fields as follows:
         for(i=0;i< inode.size; i++)
         freeblockList[ inode.blockPointer[i] ] = 0;

         Step 3: mark inode as free
         Set the "used" field to 0.

         Step 4: Write out the inode and free block list to disk
          Move the file pointer to the start of the file
         Write out the 128 byte free block list
         Move the file pointer to the position on disk where this inode was stored
         Write out the inode

    } // End Delete*/
};

int main(){
    // open up the input data test file
    char diskName[16];
    char command;
    char filename[16];
    int filesize;

    std::ifstream inputFile;
    inputFile.open("sample-test.txt");

    if (inputFile.fail()){
        std::cout << "[ERROR] Failed to open input file." << std::endl;
        exit(1);
    }

    inputFile >> diskName;
    std::cout << "Disk Name: " << diskName << std::endl;
    myFileSystem fs;
    fs.theFileSystem(diskName);
    fs.initSuperBlock();

    char buf[1024];
    for (int i = 0; i < 1024; i++){
        buf[i] = '1';
        if (i == 4){
            buf[0] = 'S';
            buf[1] = 'T';
            buf[2] = 'A';
            buf[3] = 'R';
            buf[4] = 'T';
        }
        if (i == 1023){
            buf[1021] = 'E';
            buf[1022] = 'N';
            buf[1023] = 'D';
        }
    }

    // Now that we have the disk name, we will recieve commands in the format "C file1.c 3"
    int i = 0;
    while(!inputFile.eof()){
        std::cout << "--------------------------------------------------" << std::endl;
        inputFile >> command;
        if (command != 'L'){
            inputFile >> filename;
            if (command != 'D'){
                inputFile >> filesize;
            }
        }
        i += 1;

        switch(command) {
            case 'C': // create a file with this name and this size
                std::cout << i << ". " << command << " " << filename << " " << filesize << std::endl;
                fs.create(filename, filesize);
                break;

            case 'L': // list the names of all files in the file system and their sizes
                std::cout << i << ". " << command << std::endl;
                fs.ls();
                break;

            case 'W':
                std::cout << i << ". " << command << " " << filename << " " << filesize << std::endl;
                fs.writeBlockToFile(filename, filesize, buf);
                break;

            case 'D':
                std::cout << i << ". " << command << " " << filename << std::endl;
                break;

            case 'R':
                std::cout << i << ". " << command << " " << filename << " " << filesize << std::endl;
                fs.readBlockFromFile(filename, filesize, buf);
                break;

            default:
                std::cout << command << " is not a valid command." << std::endl;
        }
    }
}
