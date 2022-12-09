# Unix-like File System
```Sammy El-Sherif``` ```Operating Systems``` ```Lab 3```

---

## File System Architecture
* The file system resides on a disk that is 128KByes in size.
* There is only one root directory. No subdirectories are allowed.
* The file system supports a maximum of 16 files.
* The maximum size of a file is 8 blocks; each block is 1KBytes in size.
* Each file has a unique name, and the file name can be no longer than 15
  characters (plus a null byte).

### Disk [128 KB = Super Block + Data Blocks]
The disk is an executable that we are able to read and write data from. The disk has a storage capacity of 128KB.

### Super Block [1KB]
The super block contains the free block list and all 16 inodes.

* The first 128 bytes store the free block list. Each entry in this list indicates
  whether the corresponding block is free or in use (if the i-th byte is 0, it indicates
  that the block is free, else it is in use). Initially, all blocks except the superblock
  are free.
* Immediately following the free block list are the 16 index nodes (inode), one for
  each file that is allowed in the file system. Initially, all inodes are free.

* Each inode stores the following information:
    * inode = 56 Bytes total
        * char name [16];      // file name with max of 15 chars + '\0'
        * int size             // file size (in number of blocks, 1KB each)
        * int blockPointers[8] // direct block pointers (1-127 in free block list)
        * int used             // 0 -> inode is free; 1 -> in use

The inodes take up 896 bytes of the super block, succeding the free block list -> SuperBlock[127-1024]
* 1024 (1KB, first data block) - 128 (free block list) = 896B (space for inodes)  
* 896 / 16 (# of inodes) = 56B -> Size of each inode

### Data Blocks [1KB each, 127 KB total]
Following the super block is all of our data blocks. Notice that the free block list which has 128 bytes indicating whether a data block is used
or not. Our block pointers can now correspond with these 128 bytes so that when we read or write based on the block pointers, we can multiply the pointer 
that ranges from 1-128 to get the position of which data block to write to on disk.

---

# Implemented Functions

## create(char filename[16], int filesize)
Creates a new file with the name 'filename' and 'filesize' many blocks. 
Assumption: (The file size is defined at file creation time and the file will not grow or shrink)

### To create a file:
1. Scan and check if there is sufficient space on the disk for this file by searching the free block list
2. Search for a free inode on the disk
3. Allocate the data blocks by going back through the free block list and setting filesize many 0's to 1's and also updating block pointers to those positions


## deleteFile(char filename[16])
Delete the file with 'filename' from the disk by first setting its used byte to 0.
Then, parse through the free block list and use this block pointers, which are within the inode for this file, 
the free the correlating blocks in the free block list. This way, even though the data blocks haven't been deleted,
the next create with a filesize < deleted file size, it will take up those spots in the free block list and overwrite
the data from this file that is being deleted.

### To delete a file:
1. Find the inode for this file
2. Free the blocks in the free block list corresponding with this files block pointers
3. Mark the inode as free ('0')

## readBlockFromFile(char filename[16], int blockNum, char buf[1024])
Red the specified blockNum from the file with 'filename' in the buffer 'buf'.

### To read in a block:
1. Locate the inode for 'filename'
2. Read in the specified block (the blockNum, which should be within this file's inode block pointers)

## writeBlockToFile(char filename[16], int blockNum, char buf[1024])
Write the data in the buffer to the specified block in this file.

### To write a block to a file:
1. Locate the inode for 'filename'
2. Write to the blockNum of that inode if the blockNum < inode.size (# of blocks)
3. The block pointers of this inode will be between 1-127, which correlates with the number of data blocks we have. So, we can multiply that by 1KB to also write its data.

## ls(void) : list the names of all files in the file system and their sizes

---

## Helper Functions

### initSuperBlock()
Set the first byte in the free block list to 1 since it is always taken up by the super block
Set all other bytes 1-127 to free = '0'

---

# Helpful Functions for Running the program
Use make to build the executable named 'filesystem' and make clean to remove that executable:
```make```
```make clean```

Run the program with ```./filesystem```

If you ever want to delete the disk and start fresh (assuming the disk name is mydisk0:
```rm mydisk0```
```./make-disk mydisk0```

Don't forget to change the permission of this file to "executable" using the Linux command,
```chmod +x make-disk```

Describe the file system
```less make-disk0```
