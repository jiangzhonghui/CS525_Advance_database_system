CS 525----Assignment 1---STORAGE MANAGER------------------

The objective of this assignmnet is to implement a Storage Manager that is capable of reading blocks from a file on the disk into the memory and writing blocks from memory to a file on the disk.

-----Group Members-----
Deep Patel (A20446182)
Baltej Singh(A20444488)
Kripa Patel(A20449132)

-----------------------
Running Instructions:

To make/compile files:

Step 1:  make -f MakeFile
Step 2: ./test_assign1_1


WHEN THE FILE IS FIRST OPENED, THE CURRENT PAGE POSITION IS THE FIRST PAGE IN THE FILE,CURRENT PAGE POSITION = 0.

-----------------------

Functions Description:
------File Manipulation Functions------

 1.createPageFile() - This function creates a new page file of 1 page, i.e. PAGE_SIZE=1 and fills it with '\0' bytes.
  In this function, calloc() is used to allocate a memory block of PAGE_SIZE and fopen() functionis used to create the file.
  If the file is created successfully, RC_OK is returned.
  
  2.openPageFile() - This function is used to open an existing page file. If the file does not exist, RC_FILE_NOT_FOUND is returned otherwise it returns RC_OK. 
  To point the file pointer to the last location of the file, fseek() function is used.Total N umber of Pages are calculated by dividing the length of the file by the page size
  Once the file is opened, the information about the opened file is  initialized to the fields(file name, total number of pages and current page position) of the file handler.
      
  3.closePageFile() - This function is used to close an already opened file. fclose() funtion is used used to close the file. It returns RC_OK if the page file is closed successfully, otherwise RC_FILE_NOT_FOUND is returned.
  
  4.destroyPageFile() - this function is used  to delete an already created page file. remove function is used to delete the file. it returns RC_OK on successful deletion of the page file and RC_FILE_NOT_FOUND otherwise.

--------Read Functions------

1. readBlock()- This function checks for the page number, that is, if it not greater than the total number of pages. 
If the page number(pageNum) is greater than the total number of pages, it throws an error and returns RC_READ_NON_EXISTING_PAGE.
Otherwise, sets the file pointer position to the page which is to be read. fseek() function is used.

2. getBlockPos()- This function returns the current page poition in a file, pointed by the file handle.

3. readFirstBlock()- This function is used to read page from the first block. It checks if the file handle contains a value or not. If the file handle is empty, it returns RC_READ_NON_EXISTING_PAGE.

4. readPreviousBlock()- It also checks if the file handle contains a value or not. If the file handle contains a value then it sets the pointer position to a previous block position.
If the previous block is not within the PAGE_SIZE limit, it throws an error. Else,it reads the previous block of the file.

5. readCurrentBlock()- It checks if the file handle contains a value or not. If the file handle contains a value, then get the position of the current block to read using getBlockPos() function and reads the current page of the file. 

6. readNextBlock()- This function checks if the file handle contains a value or not, it points to the next block in case the file handle contains a value. If the next block is not within the PAGE_SIZE limit, it throws an error. Else,it reads the next block of the file.

7. readLastBlock()- This function checks if the file handle contains a value or not, it points to the last block if the file handle contains a value. If the last block is not within the PAGE_SIZE limit, it throws an error. Else,it reads it.


--------Write Functions------

1. writeBlock()- This function checks the page number, thatis, if it is less than zero or greater than the total number of pages. If it is either of the case, it throws an error. Else,it checksif file handle contains a value or not, if it is null an error is thrown.
Otherwise, set the pointer position to the pageNum, the page where we have to write. Then it updates the current position to the page number that we just write i.e. pageNum and update the totalNumPages value by 1.

2. writeCurrentBlock()- This function is used for writing at the cureent position. getBlockPos() function is used to get the current page position.If the current block is successfully written, it returns RC_OK, else it returns RC_WRITE_FAILED.

3. appendEmptyBlock() - This function is used to increase the number of pages by appending an empty page at the end.

4. ensureCapacity() - This Function checks if the total number of pages are less than the number of pages(numberOfPages) passed in the function argument.If it is less, then we find how many pages we have to append.Then create the remaining number of pages by running a loop and calling the appendEmptyBlock() function in the loop.
