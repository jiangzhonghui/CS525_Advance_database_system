

CS 525----Assignment 2---BUFFER MANAGER------------


The objective of this assignmnet is to implement a Buffer Manager that manages a buffer of a fixed number pages in the memory and performs reading/flushing to disk and replaces the blocks (flushing blocks back the to disk to make space for reading new blocks from disk) 

-----Group Members-----

 Deep Patel (A20446182) 
Baltej Singh(A20444488) 
Kripa Patel(A20449132)(Group Leader)

-----Running Instructions:

To make/compile files:

Open Terminal

Go to the path where files are extracted

Run the below command: 

To clean previous object files : make clean

To run all the files: make

Run test case 1 : ./test1

Run test case 2 : ./test2

To remove object files, run make clean command

An additional test file is created, 'test_assign2_2.c' to test LFU replacement strategy.
 ------Buffer Pool Functions------
 Storage Manager from Assignment 1 is used to perform the operations on the page file on the disk. (The page file is present on the disk and the Buffer Pool is created in the memory). 

1.initBufferPool() - 
This function creates a new Buffer pool in the memory.
The size of the buffer is defined the numPages parameter, that is the number of pages that can be stored in the buffer pool.
Pages from the file named pageFileName are cached in the pool.
strategy represents the various page replacement strategies used by the buffer pool (LRU, Clock, FIFO, LFU).
startData is used to pass any parameters used by the page replacement strategy.
The read/write variable are initialized to 0. 

2.shutdownBufferPool() -
This function is used to Shutdown/destroy the buffer pool.
It calls the forceFlushPool function to write all the dirty pages to the disk.
It frees all the resources/memory space that is used by the Buffer Manager.

3.forceFlushPool() - 
This fuction writes all the dirty pages (with fix count 0) from the buffer pool to the disk.
The written pages are reset to "not dirty" i.e isDirty=0 and WriteIo is incremented.

--------Page Management Functions------

1.pinPage()- 
This function pins the page with page number pageNum, i.e., it reads the page file from the disk and stores it in the buffer pool.
It checks if the buffer is full, and if the buffer pool is full it calls one of the replacement strategy.
Pages are read and pinned using readBlock() and the count variable of the page FrameCount is incremented.
2.unpinPage()- 
This function iterates through the page frames in the buffer pool and finds the page to be unpinned.
Once the page to be unpinned is located, the its fix_cnt is decremented by 1, i.e., the client is no longer using that page.
3.makeDirty()-
This function is used to set the isDirty of the modified page to 1.
The pageNum is used to locate the page by iteratively checking each page in the buffer pool and when the page id found, isDirty is set 1 for that page.
4.forcePage()- 
This function writes the content of the specified page back to the page file on the disk.
It use the pageNum to locate the specified page by checking all the page frames in the buffer by iterating through it. 
Storage Manager function WriteBlock is used to write the content of the page frame to the page file on the disk.
Once the page frame is written back to the disk, the isDirty is set to 0.

--------Statistics Functions------
1.getFrameContents()-
This function returns an array of PageNumbers.
It returns constant NO_PAGE, if ethere are no pages currently in the buffer.
2.getDirtyFlags()-
This function returns an array of bools (array size = numPages), representing the dirty status of pages in the buffer.
We get the isDirty value of the page frames present in the buffer pool by iterating over all the page frames in the buffer pool.
3.getFixCounts() - 
This function is returns an array of ints (array size = numPages).
It returns an array of the values of the fix_cnt of the page frame present in the buffer pool by iterating over all the page frame in the buffer pool.
4.getNumReadIO() - 
This function returns the total number (count) of IO reads performed by the buffer pool i.e. the number of pages that have been read from disk since the buffer pool has been initialized.
5.getNumWriteIO() -
This function returns the total number (count) of IO writes performed by the buffer pool i.e. the number of pages written to the disk since the buffer pool has been initialized.

---------Page Replacement strategy Functions (FIFO, LFU, LRU,  CLOCK)-------------
FIFO: This is the most basic page replacement strategy and uses a queue, in which the page file which comes first in the buffer pool is in the front of the queue and it is replaced first when the buffer pool is full. 

LFU: The LFU algorithm looks for the page that was used the least frequently or in other words, used least number of times as compared to the other page frames present in the buffer pool and removes it. While using LFU, we need to find the position of the page frame that has the lowest reference number.Once the page with least reference is found, the contents of the page frame is written to the page file on the disk and a new page is addded.

LRU: The LRU algorithm looks for the page Least Recently used or in other words, the page frame that hasn't been used for a long time (least recent) and removes it from the buffer pool. It keeps track of the that have been pinned (used by the client). Once the page with least count of hits is found, its contents are written to the page file on the disk and a new page is added. 

CLOCK: Clock algorithmis used to keep track of the last added page frame in the buffer pool.A clock pointer is used as a counter to point to the page frames in the buffer pool.
