# Architecture of the File System

---

# Disk
Disk = 128 1KB Data Blocks
Disk[0] = Super Block



## Super Block (1KB = 1024B): Data Block #0
SuperBlock[0-127] = Free Block List

### Free Block List: Represents the Status of Data Blocks
Each byte within this 128 byte list reflects the status of all the files on the disk

Init: "Initially, all blocks except the superblock are free", 
therefore the first byte in the superblock should always be 1


## inode's
SuperBlock[127-1024]
1024-128 = 896 -> 896/16 = 56 -> Each inode is 56B in size











---

## You will create a Unix-like simple file system as follows:
* The file system resides on a disk that is 128KByes in size.
* There is only one root directory. No subdirectories are allowed.
* The file system supports a maximum of 16 files.
* The maximum size of a file is 8 blocks; each block is 1KBytes in size.
* Each file has a unique name, and the file name can be no longer than 15
characters (plus a null byte).

## The exact structure of the superblock is as follows:
* The first 128 bytes store the free block list. Each entry in this list indicates
whether the corresponding block is free or in use (if the i-th byte is 0, it indicates
that the block is free, else it is in use). Initially, all blocks except the superblock
are free.
* Immediately following the free block list are the 16 index nodes (inode), one for
each file that is allowed in the file system. Initially, all inodes are free.
Each inode stores the following information: 


### Free Block List
* If the ith bite is 0, it indicates that block is free
* Its just a mapping for whether or not a file is available
* 16 files, one int to represent each (16 * 8 = 128kb freeblocklist)

freeblocklist[16] = [ 0 | 1 | 0 | 0 | 1 ] // each block is


less make-disk0

Don't forget to change the permission of this file to "executable" using the Linux command, 
```chmod +x make-disk``` 
after you download it into the csegrid before you run it to create your disk.


less mydisk

