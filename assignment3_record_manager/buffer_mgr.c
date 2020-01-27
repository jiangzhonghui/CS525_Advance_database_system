#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "buffer_mgr_stat.h"
#include "dt.h"
#include "dberror.h"
#include "stdio.h"
#include "stdlib.h"
#include "pthread.h"


static pthread_mutex_t mutex_init = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_unpin = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_pin = PTHREAD_MUTEX_INITIALIZER;


typedef struct PageFrame
{
	int f_num;
	int pageNum;
	BM_PageHandle *bph; 
	bool isDirty; 
	int FrameCount; 
	int freq_used; 
	int fix_cnt; 
	struct PageFrame *nextFrame;
}PageFrame;

typedef struct BM_BufferPoolData
{	
	int ReadIO; 
	int WriteIO;  
	SM_FileHandle *fh; 
	PageFrame *currentnode; 
	PageFrame *search; 
	PageFrame *currentFrame; 
	PageFrame *startFrame;
}BM_BufferPoolData;


RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)
{
	BM_BufferPoolData *data1=(BM_BufferPoolData *)bm->mgmtData;
	bm->mgmtData =NULL;
	check_cd((bm->mgmtData != NULL),
	{
		return RC_BUFFER_POOL_ALREADY_INIT;
	},
	{
	
		pthread_mutex_lock(&mutex_init);
		data1 =(BM_BufferPoolData *)malloc(sizeof(BM_BufferPoolData));
		data1->fh = (SM_FileHandle *)malloc(sizeof(SM_FileHandle));
		if(data1->fh == NULL )
		{
			
			free(data1->fh);
			data1 -> fh = NULL;
			free(data1);
			data1 = NULL;
			pthread_mutex_unlock(&mutex_init);// Unlock mutex while returning
			return RC_FILE_HANDLE_NOT_INIT;
		}
		
		if(openPageFile(pageFileName, data1->fh) != RC_FILE_NOT_FOUND )
		{
		(*data1).startFrame = NULL;
		(*data1).currentFrame = NULL;
		(*data1).currentnode = NULL;
		(*data1).search = NULL;
		(*data1).ReadIO = 0;
		(*data1).WriteIO = 0;
		(*bm).pageFile = pageFileName;
		(*bm).numPages = numPages;
		(*bm).strategy = strategy;
		(*bm).mgmtData = data1;
		pthread_mutex_unlock(&mutex_init); // Unlock mutex while returning
		return RC_OK;

			
		}
		pthread_mutex_unlock(&mutex_init);// Unlock mutex while returning
			return RC_FILE_NOT_FOUND;
	
		});
	

}
RC shutdownBufferPool(BM_BufferPool *const bm)
{
	BM_BufferPoolData *data=(BM_BufferPoolData *)bm->mgmtData;
	
	if(bm->mgmtData ==NULL)
	{
		return RC_BUFFER_POOL_NOT_INIT;
	}
	else if (bm->mgmtData !=NULL )
	{
		if(data ->startFrame ==NULL)
		{
			free(data->fh);
			data->fh = NULL;
			free(bm->mgmtData);
			bm->mgmtData = NULL;
			return RC_OK;
		}
		data->currentFrame = data->startFrame;
		for(;data->currentFrame != NULL;)
		{
			data->currentFrame->fix_cnt = 0;
			data->currentFrame = data->currentFrame->nextFrame;
		}
		int ab = forceFlushPool(bm);

		//Release the memory
		free(data->fh);
		data->fh = NULL;
		free(bm->mgmtData);
		bm->mgmtData = NULL;

		return RC_OK;

	}
}
RC forceFlushPool(BM_BufferPool *const bm)
{
	BM_BufferPoolData *data=(BM_BufferPoolData *)bm->mgmtData;
	if(data == NULL)
	{
		return RC_BUFFER_POOL_NOT_INIT;
	}
	else if(data !=NULL)
	{
		data->currentnode=data->startFrame;
		check_cd((data->currentnode == NULL),
		{
			return RC_BUFFER_POOL_NULL;
		},
		{
			for(;(*data).currentnode != NULL;)
			{
				if(data->currentnode->isDirty && data->currentnode->fix_cnt == 0 )
{
if(writeBlock(data->currentnode->pageNum,data->fh,data->currentnode->bph->data) == RC_OK)
						
						{	
							data->currentnode->isDirty=0; //change the isDirty page
							data->WriteIO +=1;
						}
					
			}	
			(*data).currentnode=data->currentnode->nextFrame;
			}
			return RC_OK;
		});
			
		
	}

}



// Buffer Manager Interface Access Pages
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	BM_BufferPoolData *data=(BM_BufferPoolData *)bm->mgmtData;
	check_cd((data ==NULL),
	{		return RC_BUFFER_POOL_NOT_INIT;},

	{
		pthread_mutex_lock(&mutex_init);
		(*data).currentnode=(*data).startFrame;
		for(;data->currentnode !=NULL;)
		{
			if(data->currentnode->pageNum==page->pageNum)
				break;
			data->currentnode=data->currentnode->nextFrame;
		}
		data->search = data->currentnode;
		check_cd((data->search == NULL),
		{
			return RC_BUFFER_POOL_ERROR_IN_MARKDIRTY;
					
		},
		
		{
			data->search->isDirty=1; //mark the page as isDirty
			return RC_OK;

		});
		
					
	});
	
	

}
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	BM_BufferPoolData *data=(BM_BufferPoolData *)bm->mgmtData;
	if(data ==NULL)
		return RC_BUFFER_POOL_NOT_INIT;
	else 
	{
		pthread_mutex_lock(&mutex_init);
		(*data).currentnode=(*data).startFrame;
		for(;data->currentnode !=NULL;)
		{
			if(data->currentnode->pageNum==page->pageNum)
				break;
			data->currentnode=data->currentnode->nextFrame;
		}
		data->search = data->currentnode;
		check_cd((data->search == NULL),
		{
			pthread_mutex_unlock(&mutex_unpin);
			return RC_BUFFER_POOL_ERROR_IN_UNPINPAGE;		
		},				
		{
			data->search->fix_cnt-=1;
			pthread_mutex_unlock(&mutex_unpin);
			return RC_OK;
		});
	}
	

}
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	BM_BufferPoolData *data=(BM_BufferPoolData *)bm->mgmtData;
	check_cd((data ==NULL),
		{return RC_BUFFER_POOL_NOT_INIT;}, 
	{
		pthread_mutex_lock(&mutex_init);
		data->currentnode=data->startFrame;
		for(;data->currentnode !=NULL;)
		{
			if(data->currentnode->pageNum==page->pageNum)
				break;
			data->currentnode=data->currentnode->nextFrame;
		}
		(*data).search = (*data).currentnode;
		if(data->search ==NULL)
				return RC_OK;
		else if(writeBlock(data->search->pageNum,data->fh,data->search->bph->data) == RC_OK)
			{	
				data->search->isDirty=0;
				data->WriteIO +=1;
				pthread_mutex_unlock(&mutex_init); // Unlock mutex while returning
				return RC_OK;	
			}

	});
	
	
}

int FindEmpty(BM_BufferPool *bm)
{
   	int i;
	PageNumber *pageination=(PageNumber *)malloc(sizeof(PageNumber)*bm->numPages);
    BM_BufferPoolData *data=(BM_BufferPoolData *)bm->mgmtData;
    (*data).currentnode=(*data).startFrame;
    for(i=0;data->currentnode != NULL;)
    {
        if(data->currentnode->f_num != i)
        {
            pageination[i]=+1;
			return i;
        }
	i++;
        data->currentnode=data->currentnode->nextFrame;
    }

    check_cd((i >= bm->numPages),
    {
        return -1;
    },
    
    {
        return i;
    });
}



int replacementStrategy(BM_BufferPool *bm, BM_PageHandle *page, PageNumber pageNum)
{
    int a;
    BM_BufferPoolData *data=(BM_BufferPoolData *)bm->mgmtData;
	int high = 0;
    data->currentnode = data->startFrame;
    data->search = data->startFrame;
    while(data->currentnode != NULL)
    {
        if(data->currentnode->FrameCount > high)
        {
            if(data->currentnode->fix_cnt == 0)
            {
                high = data->currentnode->FrameCount;
                data->search = data->currentnode;
            }
        }
        data->currentnode = data->currentnode->nextFrame;
    }
    if(data->search->isDirty == 1)
    {
        if(writeBlock(data->search->pageNum,data->fh,data->search->bph->data) == RC_OK)
        {
            data->search->isDirty = 0;
            data->WriteIO += 1;
        }
    }
	data->currentnode = data->startFrame; 
	for(;data->currentnode != NULL;)
	{
		data->currentnode->FrameCount = data->currentnode->FrameCount + 1;
		data->currentnode = data->currentnode->nextFrame;
	}
    int status=readBlock(pageNum,data->fh,data->search->bph->data);
    if(status == RC_OK)
    {
        data->search->bph->pageNum = pageNum;
        data->search->pageNum = pageNum;
        data->search->isDirty = 0;
        data->search->FrameCount = 1;
        data->search->freq_used = 1;
        data->search->fix_cnt = 1;
        page->data = data->search->bph->data;
        page->pageNum = pageNum;
        return RC_OK;
    }
    return status;
}

int pinPageLFU(BM_BufferPool *bm, BM_PageHandle *page, PageNumber pageNum)
{
    int status;
    BM_BufferPoolData *data=(BM_BufferPoolData *)bm->mgmtData;
   
	int low = 999;
	int high = 0;
    data->currentnode = data->startFrame;
    data->search = data->startFrame;
    while(data->currentnode != NULL)
    {
        if(data->currentnode->FrameCount > high)
        {
            if(data->currentnode->fix_cnt == 0)
            {
                high = data->currentnode->FrameCount;
                data->search = data->currentnode;
            }
        }
        data->currentnode = data->currentnode->nextFrame;
    }
    int highCount = data->search->FrameCount;
	
    data->currentnode =data->startFrame;
    data->search = data->startFrame;
    while(data->currentnode != NULL)
    {
        if(data->currentnode->freq_used <= low)
        {
            if(data->currentnode->fix_cnt == 0 && data->currentnode->FrameCount <= highCount && data->currentnode->FrameCount != 1)
            {
                low = data->currentnode->freq_used;
                data->search = data->currentnode;
            }
        }
        data->currentnode = data->currentnode->nextFrame;
    }
    if(data->search->isDirty == 1)
    {
        status = writeBlock(data->search->pageNum,data->fh,data->search->bph->data);
        if (status == RC_OK)
        {
            data->search->isDirty = 0;
            data->WriteIO += 1;
        }
    }
	data->currentnode = data->startFrame; 
	while(data->currentnode != NULL)
	{
		data->currentnode->FrameCount = data->currentnode->FrameCount + 1;
		data->currentnode = data->currentnode->nextFrame;
	}
    status = readBlock(pageNum,data->fh,data->search->bph->data);
    if(status == RC_OK)
    {
        data->search->bph->pageNum = pageNum;
        data->search->pageNum = pageNum;
        data->search->isDirty = 0;
        data->search->FrameCount = 1;
        data->search->freq_used = 1;
        data->search->fix_cnt = 1;
        page->data = data->search->bph->data;
        page->pageNum = pageNum;
        return RC_OK;
    }
    return status;
}
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
{
	BM_BufferPoolData *data=(BM_BufferPoolData *)bm->mgmtData;	
	if(bm->mgmtData == NULL) return RC_BUFFER_POOL_NOT_INIT;
	else
	{	
		pthread_mutex_lock(&mutex_pin);//Lock mutex while returning
		if(pageNum >= data->fh->totalNumPages)
		{
			printf("Creating missing page %i\n", pageNum);
			int e = ensureCapacity(pageNum + 1, ((BM_BufferPoolData *)bm->mgmtData)->fh);
		}
		data->currentnode=data->startFrame;
		for(;data->currentnode !=NULL;)
		{
			if(data->currentnode->pageNum==pageNum)
				break;
			data->currentnode=data->currentnode->nextFrame;
		}
		data->search = data->currentnode;
		data->currentnode = data->startFrame;
		for(;data->search;)
		{
			if(data->search->pageNum == pageNum)
			{
				data->currentnode = ((BM_BufferPoolData *)bm->mgmtData)->search;
				break;
			}
			data->search = data->search->nextFrame;
		}
		check_cd((data->search != data->currentnode || data->search == 0),
		{
			int empty = FindEmpty(bm);
			check_cd((empty != -1),
			{ int i = 0;
  				  data->currentnode = data->startFrame;
 				   for(;data->currentnode != NULL;i++)
 				   {
        
 				       data->currentnode=data->currentnode->nextFrame;
 				   }
  					  

				check_cd((i != 0),
				{


				        data->currentFrame->nextFrame = (PageFrame *)malloc(sizeof(PageFrame));
					data->currentFrame = data->currentFrame->nextFrame;
					data->currentFrame->bph = MAKE_PAGE_HANDLE();
					data->currentFrame->bph->data = (char *) malloc(PAGE_SIZE);
					int d = readBlock(pageNum,data->fh,data->currentFrame->bph->data);
					check_cd((d != RC_OK),
					{
						printf("Pin Page failed because of: %d \n", d);
						pthread_mutex_unlock(&mutex_pin); // Unlock mutex while returning
						free(data->currentFrame->nextFrame);
						return RC_BUFFER_POOL_ERROR_IN_PINPAGE;
					
						
					},
					
					{	page->data =data->currentFrame->bph->data;
						page->pageNum = pageNum;
						data->currentFrame->bph->pageNum = pageNum;
						data->currentFrame->f_num = empty;
						data->currentFrame->isDirty = 0;
						data->currentFrame->FrameCount = 1;
						data->currentFrame->freq_used = 1;
						data->currentFrame->fix_cnt = 1;
						data->currentFrame->pageNum = pageNum;
						data->currentFrame->nextFrame = NULL;
						data->currentnode = data->startFrame; 
						for(;data->currentnode != NULL;)
						{
							data->currentnode->FrameCount = data->currentnode->FrameCount + 1;
							data->currentnode = data->currentnode->nextFrame;
						}
						data->currentFrame->FrameCount = 1;
						
					});
				},
				{
					data->startFrame = (PageFrame *)malloc(sizeof(PageFrame));
					data->startFrame->bph = MAKE_PAGE_HANDLE();
					data->startFrame->bph->data = (char *) malloc(PAGE_SIZE);
					int status = readBlock(pageNum,data->fh,data->startFrame->bph->data);
					check_cd((status != RC_OK),
					{
						printf("Pin Page failed because of: %d \n", status);
						pthread_mutex_unlock(&mutex_pin); 
						free(data->startFrame);
				
						return RC_BUFFER_POOL_ERROR_IN_PINPAGE;
					
					},
					{
						page->data = data->startFrame->bph->data;
						page->pageNum = pageNum;
						data->startFrame->bph->pageNum = pageNum;
						data->startFrame->f_num = empty;
						data->startFrame->isDirty = 0;
						data->startFrame->FrameCount = 1;
						data->startFrame->freq_used = 1;
						data->startFrame->fix_cnt = 1;
						data->startFrame->pageNum = pageNum;
						data->startFrame->nextFrame = NULL;
						data->currentFrame = data->startFrame;	
						
					});
					
				
				});

			},

			{
				int status;
				if(bm->strategy == RS_LFU)
				{
					status = pinPageLFU(bm, page, pageNum);//implementing LFU strategy
					
				}
				else
				{
					status = replacementStrategy(bm, page, pageNum);//Implementing the Specific Strategy 
					
				}
				if(status != RC_OK)
				{
					printf("Pin Page failed because of: %d \n", status);
					pthread_mutex_unlock(&mutex_pin);// Unlock mutex while returning
				
					return RC_BUFFER_POOL_ERROR_IN_PINPAGE;
				}
			});
		},
		{
			data->search->fix_cnt += 1;
			page->data = data->search->bph->data;
			page->pageNum = pageNum;
			if(bm->strategy == RS_LRU)
			{	data->currentnode = data->startFrame; 
				for(;data->currentnode != NULL;)
				{
					data->currentnode->FrameCount = data->currentnode->FrameCount + 1;
					data->currentnode = data->currentnode->nextFrame;
				}
				data->search->FrameCount = 1;
			}
			if(bm->strategy == RS_LFU)
			{			
				data->search->freq_used += 1;
			}
			pthread_mutex_unlock(&mutex_pin);
			return RC_OK;
			
		});
		data->ReadIO += 1;
		pthread_mutex_unlock(&mutex_pin);
		return RC_OK;
	}

	
}

// Statistics Interface
PageNumber *getFrameContents (BM_BufferPool *const bm)
{
	int x=0;
	int page_xa = sizeof(PageNumber)*(*bm).numPages;
	PageNumber *pg=(PageNumber *)malloc(sizeof(PageNumber)*bm->numPages);
	BM_BufferPoolData *data=(BM_BufferPoolData *)bm->mgmtData;
	check_cd((data ==NULL),
	{		return RC_BUFFER_POOL_NOT_INIT;},
	{
		page_xa++;
		data->currentnode = data->startFrame;
		check_cd((data->currentnode == NULL),
		{		return NO_PAGE;},
		
		{
			while(x< bm->numPages)
			{
				pg[x]=-1;
				x++;
			}
			x=0;
			for(;data->currentnode!=NULL;)
			{
				pg[x] = data->currentnode->pageNum;
				x++;
				data->currentnode=data->currentnode->nextFrame;

			}
		});
	});
	return pg;	
}
bool *getDirtyFlags (BM_BufferPool *const bm)
{
	
	BM_BufferPoolData *data=(BM_BufferPoolData *)bm->mgmtData;
	bool *dy= (bool *)malloc(sizeof(bool)*bm->numPages);
	int a=0;
	bool ca;
	check_cd((data ==NULL),
	{	return RC_BUFFER_POOL_NOT_INIT;},
	{
		ca = TRUE;
		data->currentnode=data->startFrame;
		check_cd((data->currentnode ==NULL),
		{	return NO_PAGE;},
		{
			while(a < bm->numPages)
			{
				ca=FALSE;
				dy[a] = FALSE;
				a++;
			}
			a=0;
			for(;data->currentnode!=NULL;)
			{
				dy[a]=data->currentnode->isDirty;
				a++;
				data->currentnode=data->currentnode->nextFrame;
			}

		});	
	});
	return dy;
}
int *getFixCounts (BM_BufferPool *const bm)
{
	BM_BufferPoolData *data=(BM_BufferPoolData *)bm->mgmtData;
	int size = sizeof(int)*(bm->numPages);
	int *fc = (int *)malloc(size);
	check_cd((data == NULL),
	{
		return RC_BUFFER_POOL_NOT_INIT;
	},
	{
		data->currentnode=data->startFrame;
		if((*data).currentnode == NULL)
			return NO_PAGE;
		else
		{
		int c=0;
		bool d=TRUE;
		while( c < (*bm).numPages)
		{
			d=FALSE;
			fc[c]=FALSE;
			c++;
		}
		d=TRUE;
		c =0;
		for(;(*data).currentnode !=NULL;)
		{
			fc[c]=data->currentnode->fix_cnt;
			data->currentnode=data->currentnode->nextFrame;
			c++;
		}
		}	
	});
	return fc;
}
int getNumReadIO (BM_BufferPool *const bm)
{

	check_cd( ((*bm).mgmtData != NULL),
	{
		return ((BM_BufferPoolData *)bm->mgmtData)->ReadIO;
	},
	{return 0;});

}
int getNumWriteIO (BM_BufferPool *const bm)
{
check_cd(((*bm).mgmtData == NULL),
	{
		return 0;
	},
	{
		return ((BM_BufferPoolData *)bm->mgmtData)->WriteIO;
	});

}

