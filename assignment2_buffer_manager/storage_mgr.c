#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "storage_mgr.h"
#include "dberror.h"

FILE *filept; //file pointer declaration


extern void initStorageManager(void) {
	
	
	printf("\n Starting Storage Manager! ");
}
// creating page file
RC createPageFile(char *filename) {
	char *memblg = calloc(PAGE_SIZE, sizeof(char)); //Initialized Memory Block of size PAGE_SIZE
	filept = fopen(filename, "w+"); //Opening the file in write mode having name as filename

	check_cond((filept != NULL),
	{				//c(,,);
		memset(memblg, '\0', PAGE_SIZE);
		int page_n = PAGE_SIZE*sizeof(char);
		fwrite(memblg, sizeof(char), PAGE_SIZE, filept);
		free(memblg);
		page_n++;
		fclose(filept);
		return RC_OK;
	},
	
	{return RC_FILE_NOT_FOUND;});
		
}

RC openPageFile(char *filename, SM_FileHandle *fHandle) {
	filept = fopen(filename, "r+");		//Opening the file in read mode
	
	check_cond((filept != NULL),
	{
		fseek(filept, 0, SEEK_END);//To point the file pointer to the last location of the file fseek function is used

		/*int Lastpos = ftell(filept); 		//ftell function returns the last byte of file
		int Lengthtotal = Lastpos + 1;*/ //  Total length of the file
		int numofpage = (ftell(filept)+1) / PAGE_SIZE; //Total number of pages 

		//Initializing the file attributes like fileName, total Number of Pages and current Page Position
		(*fHandle).fileName = filename; 
		(*fHandle).totalNumPages = numofpage;
		(*fHandle).curPagePos = 0;

		rewind(filept); //rewind function sets the file pointer back to the start of the file
		return RC_OK;
	},
	{
		return RC_FILE_NOT_FOUND;
	});	
}

RC closePageFile(SM_FileHandle *fHandle) {
	
	check_cond((fclose(filept) == 0),
	
	{return RC_OK;},
	
	{return RC_FILE_NOT_FOUND;});

}

RC destroyPageFile(char *fileName) {

	check_cond((remove(fileName) == 0),
	{return RC_OK;},
		
	{return RC_FILE_NOT_FOUND;});
	
}


RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
	RC returncode;
	RC read_blocksize;

	check_cond((pageNum > fHandle->totalNumPages),	//If the page number is greater than the total no. of pages then throw an error
	{returncode = RC_READ_NON_EXISTING_PAGE;},

	{
			long int size = pageNum * PAGE_SIZE;
		fseek(filept,size, SEEK_SET); //To point the file pointer to the begining of the file fseek function is used
		read_blocksize = fread(memPage, sizeof(char), PAGE_SIZE, filept);

		//If the size of the block returned by fread() is not within the limit of the Page size then throw an error
		check_cond( (read_blocksize < PAGE_SIZE || read_blocksize > PAGE_SIZE), 
		{
		returncode = RC_READ_NON_EXISTING_PAGE;
		},

		{fHandle->curPagePos = pageNum;//Update current page position to pageNum value
		returncode = RC_OK;});
	});

	{return returncode;}
	
}

int getBlockPos(SM_FileHandle *fHandle) {
	RC block_position;
	block_position = ((*fHandle).curPagePos);	//get current page position
	return block_position;
}


RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
	RC returncode;
	check_cond((fHandle != NULL),
	{
		check_cond (((*fHandle).totalNumPages <= 0),{
			returncode = RC_READ_NON_EXISTING_PAGE;
		},
	 
		{
			fseek(filept, 0, SEEK_SET);
			RC read_first_block;
			read_first_block = fread(memPage, sizeof(char), PAGE_SIZE, filept); //fread returns the first block of size PAGE_SIZE
			fHandle->curPagePos = 0; //First page of the file has index 0

			check_cond ((read_first_block < 0 || read_first_block > PAGE_SIZE), //If the read block is not within pagesize limits, throw an error
			{returncode = RC_READ_NON_EXISTING_PAGE;},{

			returncode = RC_OK;});
		});
	},
	{
		returncode = RC_FILE_NOT_FOUND;
	});

	return returncode;
}

//-------------

RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
	RC returncode;

	check_cond ((fHandle != NULL),
	{

		RC previous_block_position;
		RC read_previous_block;

		previous_block_position = (*fHandle).curPagePos - 1; //storing the index of previous block i.e 1 less than current page position

		check_cond ((previous_block_position < 0),
		{
			returncode = RC_READ_NON_EXISTING_PAGE;
		},

		{	long int current = (*fHandle).curPagePos;
			fseek(filept, (previous_block_position * PAGE_SIZE), SEEK_SET); //fseek will point the file pointer to the start of the previous block
			read_previous_block = fread(memPage, sizeof(char), PAGE_SIZE, filept); //fread returns the read block
			(*fHandle).curPagePos = (*fHandle).curPagePos - 1; //Update the current page position reducing it by 1

			check_cond ( (read_previous_block < 0 || read_previous_block > PAGE_SIZE), //If the previous block is not within pagesize limits, throw an error
			{	returncode = RC_READ_NON_EXISTING_PAGE;},{ returncode = RC_OK;});

		});
	},
	{ 
		returncode = RC_FILE_NOT_FOUND;
	});

	return returncode;
}

//Reads the current block which the file pointer points to
RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
	RC returncode;

	check_cond ((fHandle != NULL),	
	{
		RC current_block_position;
		RC read_current_block;

		current_block_position = getBlockPos(fHandle); //get the position of the current block to read
		long int c_b_pos = current_block_position * PAGE_SIZE;
		fseek(filept, c_b_pos, SEEK_SET); //fseek will point the file pointer to the start of the current block
		read_current_block = fread(memPage, sizeof(char), PAGE_SIZE, filept); //fread returns the current block to be read
		fHandle->curPagePos = current_block_position;

		check_cond ( (read_current_block < 0 || read_current_block > PAGE_SIZE), //If the current block is not within pagesize limits, throw an error
			
		{	returncode = RC_READ_NON_EXISTING_PAGE;},
			
			{
			returncode = RC_OK;});
	},
	{ //If the fhandle is null, throw an error
		returncode = RC_FILE_HANDLE_NOT_INIT;
	});


	return returncode;
}

//Reading next block of the file
RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
	RC returncode;

	check_cond( (fHandle != NULL),
	{
		RC next_block_position;
		RC read_next_block;

		next_block_position = fHandle->curPagePos + 1; //get the position of next block 
		check_cond( (next_block_position < (*fHandle).totalNumPages),
		{

			fseek(filept, (next_block_position * PAGE_SIZE), SEEK_SET); //fseek points the file pointer to the next block
			read_next_block = fread(memPage, sizeof(char), PAGE_SIZE, filept); //fread returns the next block to be read

			(*fHandle).curPagePos = next_block_position; //Update current page position to next block index

			check_cond ( (read_next_block < 0 || read_next_block > PAGE_SIZE), //If the next block is not within pagesize limits, throw an error
			{returncode = RC_READ_NON_EXISTING_PAGE;},

				{returncode = RC_OK;});
		},
		{
		return RC_READ_NON_EXISTING_PAGE;});

	},
	{
	returncode = RC_FILE_HANDLE_NOT_INIT;}); 

	return returncode;
}


//Reading the last block of the file
RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
	RC returncode;

	check_cond ( (fHandle != NULL),
	{
		RC find_last_block_pos;
		RC read_last_block;

		find_last_block_pos = (*fHandle).totalNumPages - 1; //storing the index of the Last block i.e 1 less than total no. of pages
		long int size = find_last_block_pos * PAGE_SIZE;
		fseek(filept, size, SEEK_SET); //fseek points the file pointer to the beginning of the last block
		read_last_block = fread(memPage, sizeof(char), PAGE_SIZE, filept); //fread returns the last block to be read


		(*fHandle).curPagePos = find_last_block_pos; //Update current page position to the last block index

		check_cond( (read_last_block < 0 || read_last_block > PAGE_SIZE), //If the last block is not within pagesize limits, throw an error
		{returncode = RC_READ_NON_EXISTING_PAGE;},

			{returncode = RC_OK;});
	},
	{
		returncode = RC_FILE_NOT_FOUND;
	});

	return returncode;

}

//--------------

RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
	RC returncode;
	check_cond( (pageNum < 0 || pageNum > (*fHandle).totalNumPages), //If the pageNum is greater than totalNumber of pages, or less than zero, throw an error
	{returncode = RC_WRITE_FAILED;},

	{

	check_cond( (fHandle != NULL),
	{
		check_cond ( (filept != NULL) ,
		{ 
			check_cond( (fseek(filept, (PAGE_SIZE * pageNum), SEEK_SET) == 0) ,
			{
				fwrite(memPage, sizeof(char), PAGE_SIZE, filept);

				(*fHandle).curPagePos = pageNum; //Update current page position to the pageNum i.e. index of the block written

				fseek(filept, 0, SEEK_END);
				(*fHandle).totalNumPages = ftell(filept) / PAGE_SIZE; //update the value of totalNumPages which increases by 1

				returncode = RC_OK;
			},
			{ 
				returncode = RC_WRITE_FAILED;
			});
			
		} ,
		{ //If file is not found, throw an error
		returncode = RC_FILE_NOT_FOUND;});
		
	},
	{
	returncode = RC_FILE_HANDLE_NOT_INIT;});
	
	});
 
	return returncode;

}

//Writing current block in the file
RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
 {
	check_cond((writeBlock(getBlockPos(fHandle), fHandle, memPage) == RC_OK),return RC_OK;,return RC_WRITE_FAILED;);

}

//Append empty block to the file
RC appendEmptyBlock(SM_FileHandle *fHandle) {

	int returncode;
	check_cond ( (filept != NULL),
	{
		RC size = 0;

		char *nblock;
		nblock = (char *) calloc(PAGE_SIZE, sizeof(char)); //Create and initialize the new empty block

		fseek(filept, 0, SEEK_END); //To point the file pointer to the end of the file fseek function is used
		size = fwrite(nblock, 1, PAGE_SIZE, filept); // it will write the emptyblock at the end of the file

		check_cond( (size == PAGE_SIZE),
		{
			(*fHandle).totalNumPages = ftell(filept) / PAGE_SIZE; //update total no. of pages i.e. increase by 1
			(*fHandle).curPagePos = fHandle->totalNumPages - 1; //update current page position i.e. index is 1 less than totalNumPages
			returncode = RC_OK;
		},
		{
		returncode = RC_WRITE_FAILED;});

		free(nblock);

	} ,
	{
		returncode = RC_FILE_NOT_FOUND;
	});
	return returncode;
}


RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) {

	int PageNumber = fHandle->totalNumPages;
	int c;

	check_cond( (numberOfPages > PageNumber), //If the new capacity is greater than current capacity
	{ 
		int add_page = numberOfPages - PageNumber;
		for (c = 0; c < add_page; c++) //Add add_page no. of pages by calling the appendEmptyBlock function
			appendEmptyBlock(fHandle);

		return RC_OK;
	},
	{
		return RC_WRITE_FAILED;});
}


	
