#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<math.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<string.h>
#include "storage_mgr.h"


FILE *filept;

void initStorageManager (void) {
	printf("\n Storage Manager Booting up\n ");
	filept = NULL; // file pointer initialized
}

// Function for constructing a page file
RC createPageFile (char *fileName) {
	filept = fopen(fileName, "w+"); // Authorization to write is granted while opening the file

	// Condition to check whether the file is empty
	check_cond((filept != NULL),{

		SM_PageHandle emptyPage = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));// empty page creation in thr memory

		// writing operation in the memory.
		check_cond((fwrite(emptyPage, sizeof(char), PAGE_SIZE,filept) >= PAGE_SIZE),
		{
			printf("write successful \n");
		})
		else
			printf("write failure \n");

		//closing a file
		fclose(filept);
		// on the completion of the write function, memory is set free.
		free(emptyPage);
		return RC_OK;
	})
	else
		return RC_FILE_NOT_FOUND;
}

//function definition for opening a page file
RC openPageFile (char *fileName, SM_FileHandle *fHandle) {
	// read permission granted while opening a file
	filept = fopen(fileName, "r");
	check_cond(check_equal(filept,NULL),
	{
		return RC_FILE_NOT_FOUND;
	})
	else
	{
		//fseek(filept, 0, SEEK_END);//function to set the pointer to the end position of the page file
			struct stat fi;
	    //Assigning values to file handler.
	    (*fHandle).fileName = fileName;
	    (*fHandle).curPagePos = 0;
	    //(*fHandle).totalNumPages = (ftell(filept) + 1) / PAGE_SIZE;
			check_cond(check_less((fstat(fileno(filept), &fi)),0),
				{
					return RC_ERROR;
				})

			fHandle->totalNumPages = (fi.st_size/ PAGE_SIZE);


	    fclose(filept); //rewind function used to reset the pointer to its commencement.
	    return RC_OK;
	}
}

//function to terminate the page file
RC closePageFile (SM_FileHandle *fHandle) {
	check_cond((filept != NULL),{filept = NULL;})
	return RC_OK;
}

//destroying the generated page file
RC destroyPageFile (char *fileName) {
	filept = fopen(fileName,"r");
	check_cond(check_equal(filept,NULL),
	{
		return RC_FILE_NOT_FOUND;
	})

	remove(fileName); //delete the file after its done
	return RC_OK;
}

/*Reading Operation */
 //function for extracting the data from the block in reading mode.
 
RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
	int isSeek;
	filept = fopen(fHandle->fileName, "r");
	check_cond(check_equal(filept,NULL),
		{return RC_FILE_NOT_FOUND;})
	check_cond(((pageNum < 0) || (pageNum > fHandle->totalNumPages)),
	{return RC_READ_NON_EXISTING_PAGE;})
	long int size =pageNum * PAGE_SIZE;
	isSeek = fseek(filept, size, SEEK_SET);
	check_cond(check_equal(isSeek, 0),
	{
	// Reading the content
  // If else check_cond to check the block size
		check_cond((fread(memPage, sizeof(char), PAGE_SIZE, filept) < PAGE_SIZE),{return RC_ERROR;})
	})
	else
	{
		return RC_READ_NON_EXISTING_PAGE;
	}
	//modifying the current page position
	fHandle->curPagePos = ftell(filept);
	// Closing file.
	fclose(filept);
    return RC_OK;
}

//function for extracting the position of the current block
int getBlockPos (SM_FileHandle *fHandle) {
	RC block_position;
	block_position = ((*fHandle).curPagePos);	//get current page position
	return block_position;
}

//function from extracting the data from the first block
RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	
	filept = fopen(fHandle->fileName, "r");
	
	check_cond(check_equal(filept,NULL),{return RC_FILE_NOT_FOUND;})
	for(int i=0;i<PAGE_SIZE;i++)
	{
		char bit = fgetc(filept);
		check_cond(feof(filept),{break;})
		else
		{
			memPage[i] = bit;
		}
	}
	fHandle->curPagePos = ftell(filept);
	fclose(filept);
	return RC_OK;
}

//function for extracting the data from the previous block
RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	
	int current_page,start;
	char bit;
	//if check_cond for checking the index
	check_cond((fHandle->curPagePos > PAGE_SIZE),{
		filept = fopen(fHandle->fileName, "r");
		check_cond(check_equal(filept,NULL),{return RC_FILE_NOT_FOUND;})
		
		current_page = (fHandle->curPagePos / PAGE_SIZE);
		start = (PAGE_SIZE * (current_page - 2));

		// function to set the pointer to the previous block position.
		fseek(filept, start, SEEK_SET);
		
		for(int i = 0; i < PAGE_SIZE; i++) {
			bit = fgetc(filept);
			memPage[i] = bit;
		}

		
		(fHandle->curPagePos) = ftell(filept);
		
		fclose(filept);
		return RC_OK;
	})
	else
	{
		printf("\n No previous block as this is the first block.");
		return RC_READ_NON_EXISTING_PAGE;
	}
}

//function to read the current block
RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	int current_page,start;
	
	filept = fopen(fHandle->fileName, "r");
	
	check_cond(check_equal(filept,NULL),{return RC_FILE_NOT_FOUND;})

	
	current_page = (fHandle->curPagePos / PAGE_SIZE);
	start = (PAGE_SIZE * (current_page - 2));


	
	fseek(filept, start, SEEK_SET);
	
	for(int i = 0; i < PAGE_SIZE; i++) {
		check_cond((feof(filept)),{break;})
		memPage[i] = fgetc(filept);
	}

	//pointer set to the current page position
	fHandle->curPagePos = ftell(filept);

	
	fclose(filept);
	return RC_OK;
}

//Reading next block of the file
 RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
 	int current_page,start;
 	
	filept = (fopen(fHandle->fileName, "r"));
	
	check_cond((fHandle->curPagePos != PAGE_SIZE),{
	
	//incrementing the index of pointer to the succeeding block
		current_page = (fHandle->curPagePos / PAGE_SIZE);
		start = (PAGE_SIZE * (current_page - 2));
		
		fseek(filept, start, SEEK_SET);

		
		check_cond(check_equal(filept,NULL),{return RC_FILE_NOT_FOUND;})

		
		for(int i = 0; i < PAGE_SIZE; i++) {
			check_cond((feof(filept)),{break;})
			memPage[i] = fgetc(filept);
		}

		
		fHandle->curPagePos = ftell(filept);

		
		fclose(filept);
		return RC_OK;
	})
	else
	{
		printf("\n This is last block. Hence, next block cant be found.");
		return RC_READ_NON_EXISTING_PAGE;
	}
}

//function to execute the last block reading operation
RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
	int start;
	start = ((fHandle->totalNumPages - 1) * PAGE_SIZE);
	
	filept = (fopen(fHandle->fileName, "r"));
	
	check_cond(check_equal(filept,NULL),{return RC_FILE_NOT_FOUND;})
	
	fseek(filept, start, SEEK_SET);
	

	for(int i = 0; i < PAGE_SIZE; i++) {
		check_cond((feof(filept)),{break;})
		memPage[i] = fgetc(filept);
	}

	//decrementing the index of pointer to the preceeding block
	fHandle->curPagePos = ftell(filept);

	
	fclose(filept);
	return RC_OK;
}

/*Writing Operation */
// writing block at a given page number
RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
	int start;
	
	filept = (fopen(fHandle->fileName, "r+"));

	
	check_cond(check_equal(filept,NULL),{return RC_FILE_NOT_FOUND;})
	
	check_cond(((pageNum < 0) || (pageNum > fHandle->totalNumPages)),{return RC_WRITE_FAILED;})
	start = (pageNum * PAGE_SIZE);

	check_cond((pageNum != 0),{
		// data written to the first page.
		fHandle->curPagePos = start;
		fclose(filept);
		writeCurrentBlock(fHandle, memPage);
	})
	else
	{
		//updating the current page location to the pagenum
		fseek(filept, start, SEEK_SET);
		for(int i = 0; i < PAGE_SIZE; i++)
		{
			
			if(feof(filept) == "TRUE")
			{
				appendEmptyBlock(fHandle);
			}
			
			fputc(memPage[i], filept);
		}

		
		fHandle->curPagePos = ftell(filept);
		
		fclose(filept);
	}
	return RC_OK;
}

//exteding the file by adding an empty block
RC appendEmptyBlock (SM_FileHandle *fHandle) {
	int seek;
	
	SM_PageHandle emptyBlock = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));
	
	seek = fseek(filept, 0, SEEK_END);

	check_cond((seek != 0), {
		free(emptyBlock);
		return RC_WRITE_FAILED;
	})
	else
	{
		
		fwrite(emptyBlock, sizeof(char), PAGE_SIZE, filept);
	}
	
	free(emptyBlock);

	
	fHandle->totalNumPages = fHandle->totalNumPages + 1;
	return RC_OK;
}

//function to write to the current block
RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
    
	filept = (fopen(fHandle->fileName, "r+"));

	
	check_cond(check_equal(filept,NULL),{return RC_FILE_NOT_FOUND;})
	
	appendEmptyBlock(fHandle);
	fseek(filept, fHandle->curPagePos, SEEK_SET);

	
	fwrite(memPage, sizeof(char), strlen(memPage), filept);
	fHandle->curPagePos = ftell(filept);
	fclose(filept);
	return RC_OK;
}

//ensuring the capacity of the file

RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle) {
	
	filept = fopen(fHandle->fileName, "a");
	check_cond(check_equal(filept,NULL),{return RC_FILE_NOT_FOUND;})

	
	while(numberOfPages > fHandle->totalNumPages){
		appendEmptyBlock(fHandle);
	}
	
	fclose(filept);
	return RC_OK;
}
