#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "buffer_mgr_stat.h"
#include "dberror.h"
#include "record_mgr.h"
#include "expr.h"


typedef struct PageFrame
{
	int f_num; int pageNum; BM_PageHandle *bph; bool isDirty; int FrameCount; int freq_used; int fix_cnt; struct PageFrame *nextFrame;
}PageFrame;

typedef struct BM_BufferPoolInfo
{	
	int ReadIO; int WriteIO; SM_FileHandle *fh; PageFrame *currentNode; PageFrame *search; PageFrame *currentFrame; PageFrame *startFrame;
}BM_BufferPoolInfo;

typedef struct PrimaryKey_Record
{
    char *data; struct PrimaryKey_Record *pk_nkey;
} PrimaryKey_Record;

typedef struct RM_RecordPoolInfo
{
    BM_BufferPool *bm; int *rp_epages; PrimaryKey_Record *arraykeys; PrimaryKey_Record *currentNode; PrimaryKey_Record *currentKey;
} RM_RecordPoolInfo;

typedef struct RecordM_PoolScan
{
    Expr *condition; int pgcurrent; Record *cRecord;
} RecordM_PoolScan;

extern RC initRecordManager (void *mgmtData)
{
		return RC_OK;
}

extern RC shutdownRecordManager ()
{
		return RC_OK;
}

extern RC createTable (char *name, Schema *schema)
{
		char *schemaInfo;
		int status=createPageFile(name);
		if(status !=RC_OK)
			return status;
		else
		{
			SM_FileHandle filehandle;
			status=openPageFile(name, &filehandle);
			if(status != RC_OK)
				printf("\nFile not open in create table");
			else
			{
				int j=0, mem = 0;
				
				while(j < schema->numAttr)
				{
					if(schema->dataTypes[j] == DT_INT)
						mem += sizeof(int);
					else if(schema->dataTypes[j] == DT_FLOAT)
						mem += sizeof(float);
					else if(schema->dataTypes[j] == DT_BOOL)
						mem += sizeof(bool);
					else
						mem += schema->typeLength[j];
					j++;
				}
			
	  
			int memoryNo = mem,i=10;
			char *out,rd[10];
			out = (char *)malloc(3*memoryNo);
			sprintf(out, "%d", schema->numAttr);
			strcat(out, "\n");
			i=0;		
			while(i < schema->numAttr)
			{
				strcat(out, schema->attrNames[i]);
				strcat(out, " ");
				sprintf(rd, "%d", schema->dataTypes[i]);
				strcat(out, rd);
				strcat(out, " ");
				sprintf(rd, "%d", schema->typeLength[i]);
				strcat(out, rd);
				strcat(out, "\n");
				i++;
			}
			sprintf(rd, "%d", schema->keySize);
			strcat(out, rd);
			strcat(out, "\n");
			i=0;
			while(i < schema->keySize)
			{
				sprintf(rd, "%d", schema->keyAttrs[i]);
				strcat(out, rd);
				strcat(out, " ");
				i++;
			}
			strcat(out, "\n");
			

					schemaInfo = out;
					status=writeBlock(0, &filehandle, schemaInfo) ;
					check_cond((status != RC_OK),
					{printf("\nFile - cannot write in create table");},
					{
						schemaInfo = NULL;
						free(schemaInfo);
						return RC_OK;
					});
			}        
		}
}

extern RC openTable (RM_TableData *rel, char *name)
{
		int status;
		RM_RecordPoolInfo *recordmanage = (RM_RecordPoolInfo *)malloc(sizeof(RM_RecordPoolInfo));
		recordmanage->bm = MAKE_POOL();
		recordmanage->bm->mgmtData = NULL;
		status=initBufferPool(recordmanage->bm, name, 4, RS_FIFO, NULL);
		if(status !=RC_OK)
			return status;
		else
		{
			recordmanage->rp_epages = (int *) malloc(sizeof(int));
			recordmanage->rp_epages[0] = ((BM_BufferPoolInfo *)(recordmanage->bm)->mgmtData)->fh->totalNumPages;

			rel->name = name;
			rel->mgmtData = recordmanage;

			char rd[10],*out;
			out = (char *)malloc(10);
			SM_PageHandle pagehandle;
			SM_FileHandle filehandle;
			pagehandle = (SM_PageHandle) malloc(PAGE_SIZE);
			int i, j;
			status=openPageFile(name, &filehandle);
			if(status !=RC_OK)
				printf("Not able to open file in open table");			
			else
			{
				status=readFirstBlock(&filehandle, pagehandle); 
				if(status == RC_OK)
				{
					sprintf(out, "%c", pagehandle[0]);
					i=1;
					while(i < strlen(pagehandle))
					{
						sprintf(rd, "%c", pagehandle[i]);

						check_cond((strcmp(rd,"\n") == 0),
						{	break;},
						{strcat(out, rd);});
						i++;
					}
					int numAttr = atoi(out);
					char **attrNames = (char **) malloc(sizeof(char*) * numAttr);
					DataType *dataTypes = (DataType *) malloc(sizeof(DataType) * numAttr);
					int *typeLength = (int *) malloc(sizeof(int) * numAttr);;
					j=0;
					while(j < numAttr)
					{
						free(out);
						out = (char *)malloc(10);
						int k = 1;
						i=i+1;
						while(i < strlen(pagehandle))
						{
							sprintf(rd, "%c", pagehandle[i]);
							if(strcmp(rd," ") == 0)
							{
								if(k == 1)
								{
									attrNames[j] = (char *) malloc(2);
									strcpy(attrNames[j], out);
								}
								else if(k == 2)
								{
									dataTypes[j] = atoi(out);
								}
								k++;
								free(out);
								out = (char *)malloc(10);
							}
							else if(strcmp(rd, "\n") == 0)
							{
								typeLength[j] = atoi(out);
								break;
							}
							else
							{
								sprintf(rd, "%c", pagehandle[i]);
								strcat(out, rd);
							}
							i++;
						}
						j++;
					}
					free(out);
					out = (char *)malloc(10);
					i=i+1;
					while(i < strlen(pagehandle))
					{
						sprintf(rd, "%c", pagehandle[i]);

						check_cond((strcmp(rd,"\n") == 0),
						{	break;},
						{strcat(out, rd);});
						i++;
					}
					int keySize = atoi(out);
					int *keyAttrs = (int *) malloc(sizeof(int) *keySize);
					j=0;
					while(j < keySize)
					{
						free(out);
						out = (char *)malloc(10);
						i=i+1;
						while(i < strlen(pagehandle))
						{
							sprintf(rd, "%c", pagehandle[i]);

							if(strcmp(rd," ") == 0)
							{
								keyAttrs[j] = atoi(out);
							}
							check_cond((strcmp(rd,"\n") == 0),
							{	break;},
							{strcat(out, rd);});
							i++;
						}
						j++;
					}
					Schema *Schema1 = (Schema *) malloc(sizeof(Schema));
					Schema1->numAttr = numAttr;
					Schema1->attrNames = attrNames;
					Schema1->dataTypes = dataTypes;
					Schema1->typeLength = typeLength;
					Schema1->keyAttrs = keyAttrs;
					Schema1->keySize = keySize;
					 
					rel->schema = Schema1;
					return RC_OK;
				}
			}
			
		}
		
}

extern RC closeTable (RM_TableData *rel)
{
		RM_RecordPoolInfo *recordmanage=(RM_RecordPoolInfo *)rel->mgmtData;
		//Shut down buffer pool
		int status=shutdownBufferPool(recordmanage->bm);
		check_cond((status !=RC_OK),
		{	return status;},
		{   
		   
			recordmanage->bm->mgmtData = NULL;
			recordmanage->bm = NULL;
			recordmanage->rp_epages = NULL;
			free(recordmanage->rp_epages);
			for(recordmanage->currentNode = recordmanage->arraykeys;recordmanage->currentNode != NULL;recordmanage->currentNode=recordmanage->currentNode->pk_nkey)
			{
				recordmanage->currentNode->data = NULL;
				free(recordmanage->currentNode->data);
				
			}
			recordmanage->arraykeys = NULL;
			free(recordmanage->arraykeys);
			rel->mgmtData = NULL;
			free(rel->mgmtData);
			rel->schema = NULL;
			free(rel->schema);
			return RC_OK;
		});
		
}

extern RC deleteTable (char *name)
{
		int status=destroyPageFile(name); 
		if(status !=RC_OK){return status;}
		if(status == RC_OK)
			return RC_OK;
		
}

extern int getNumTuples (RM_TableData *rel)
{
		RM_RecordPoolInfo *recordmanage=(RM_RecordPoolInfo *)rel->mgmtData;
		Record *record = (Record *)malloc(sizeof(Record));
		RID rid;
		
		rid.slot = 0;
		int status;
		int a = 0;
		for(rid.page = 1;rid.page < ((BM_BufferPoolInfo *)(recordmanage->bm)->mgmtData)->fh->totalNumPages && rid.page > 0;(rid.page)++)
		{
			if(getRecord (rel, rid, record) == RC_OK)
			{ 
				rid.slot= 0;
				a++; //incerement tuple counter
				
			}
		}
		record = NULL;
		free(record);
		return a;// return the tuple counter
}
extern RC insertRecord (RM_TableData *rel, Record *record)
{
		RM_RecordPoolInfo *rec=(RM_RecordPoolInfo *)rel->mgmtData;
		int vp,spo=1;
		RM_RecordPoolInfo *recordmanage=(RM_RecordPoolInfo *)rel->mgmtData;
		int status;
		Value *val;
		char *seri_val;
		recordmanage->currentNode = recordmanage->arraykeys;
		status = getAttr (record, rel->schema, rel->schema->keyAttrs[0], &val);
		if(status == RC_OK)
		{
			seri_val = serializeValue(val);
			while(recordmanage->currentNode != NULL)
			{
				if(strcmp(seri_val, recordmanage->currentNode->data) == 0)
				{
					vp= 1;
					goto par;

				}
				recordmanage->currentNode = recordmanage->currentNode->pk_nkey;
			}
		}
		if(spo == 1)
		{
			check_cond((recordmanage->rp_epages[0] == 1),
			{
				recordmanage->arraykeys = (PrimaryKey_Record *)malloc(sizeof(PrimaryKey_Record));
				recordmanage->arraykeys->data = (char *)malloc(strlen(seri_val));
				strcpy(recordmanage->arraykeys->data, seri_val);
				recordmanage->arraykeys->pk_nkey = NULL;
				recordmanage->currentKey = recordmanage->arraykeys;
			},
			{
				recordmanage->currentKey->pk_nkey = (PrimaryKey_Record *)malloc(sizeof(PrimaryKey_Record));
				recordmanage->currentKey = recordmanage->currentKey->pk_nkey;
				recordmanage->currentKey->data = (char *)malloc(strlen(seri_val));
				strcpy(recordmanage->currentKey->data, seri_val);
				recordmanage->currentKey->pk_nkey = NULL;
			});
		}
		vp= 0;
		
	par:	
		if(vp == 1)
			return RC_RM_PRIMARY_KEY_ALREADY_PRESENT_ERROR; 
		 Record *record1 = (Record *)malloc(sizeof(Record));
		 RID rid;
		
		rid.slot = 0;
		for(rid.page = 1;rid.page > 0 && rid.page < ((BM_BufferPoolInfo *)(recordmanage->bm)->mgmtData)->fh->totalNumPages; rid.page = rid.page + 1)
		{
			if(getRecord (rel, rid, record1) == RC_OK)
			{   
				if(strncmp(record1->data, "deleted:", 7) == 0)
					break;
				rid.slot = 0;
				
			}
		}
		record1 = NULL;
		free(record1); //free the memory
		recordmanage->rp_epages[0] = rid.page; // setting the page number
		BM_PageHandle *pg = MAKE_PAGE_HANDLE();//making page
		//pinning page 
		status=pinPage(recordmanage->bm, pg, recordmanage->rp_epages[0]);
		if(status !=RC_OK)
			return status;
		else
		{        
			memset(pg->data, '\0', strlen(pg->data));
			sprintf(pg->data, "%s", record->data);        
			status=markDirty(recordmanage->bm, pg);
			if(status !=RC_OK)
				printf("insert record error");			
			else
			{   
			   
				 status=unpinPage(recordmanage->bm, pg);
				if(status !=RC_OK)
					printf("insert record error 1");				
				else
				{  
				status=forcePage(recordmanage->bm, pg);
					check_cond((status !=RC_OK),
					{	printf("insert record error 2");},					
					{
						record->id.page = recordmanage->rp_epages[0];
						record->id.slot = 0;
						printf("Inserting record %d\n", record->id);
					    	//pg = NULL;
						free(pg);
						rec->rp_epages[0] += 1;
						return RC_OK;
					});
				}
			}
		}
		
}
extern RC deleteRecord (RM_TableData *rel, RID id)
{
		RM_RecordPoolInfo *recordmanage=(RM_RecordPoolInfo *)rel->mgmtData;
		char tombstone[8] = "deleted:"; //soft delete for Tombstones implementation
		char *rd = MAKE_PAGE_HANDLE();  
		int i,status;
		if(id.page > 0 && id.page <=  ((BM_BufferPoolInfo *)(recordmanage->bm)->mgmtData)->fh->totalNumPages)
		{
			BM_PageHandle *pg = MAKE_PAGE_HANDLE();
			status = pinPage(recordmanage->bm, pg, id.page);//pin page
			
			if(status != RC_OK)
				printf("delete record error");
			else
			{
				Record *r = (Record *)malloc(sizeof(Record));
				RID rid;
				rid.page = id.page;
				rid.slot = id.slot;
				status = getRecord (rel, rid, r); //fetch record from the table
				if(status !=RC_OK)
					printf("delete record error 1");
				else
				{   
					RM_RecordPoolInfo *record1=(RM_RecordPoolInfo *)rel->mgmtData;
					int rvp;
					Value *val;
					char *seri_val;
					
					check_cond((getAttr(r, rel->schema, rel->schema->keyAttrs[0], &val) == RC_OK),
					{
						seri_val = serializeValue(val);
						for(record1->currentNode = record1->arraykeys;record1->currentNode != NULL;record1->currentNode = record1->currentNode->pk_nkey)
						{
							if(strcmp(seri_val, record1->currentNode->data) == 0)
							{
								record1->currentNode->data = "#####"; //Check data with '#'
								rvp=1;
							}
							
						}
					},
					{
						rvp=0;
					});
					status = rvp; 
					if(status == 0)
						return RC_RM_PRIMARY_KEY_DELETE_ERROR;
				}
				r = NULL;
				free(r);
				strcpy(rd, tombstone);
				strcat(rd, pg->data); 
				memset(pg->data, '\0', strlen(pg->data));
				sprintf(pg->data, "%s", rd); 
				status = markDirty(recordmanage->bm, pg);
				if(status != RC_OK){}
				else{
					status = unpinPage(recordmanage->bm, pg);
					if(status != RC_OK){}
					else
					{
						status = forcePage(recordmanage->bm, pg);
						check_cond((status != RC_OK),{},
						{
						   	// pg = NULL;
							free(pg);//free page
							return RC_OK;
						});
					}
				}
			}
			return status;
		}
		return RC_RM_NO_RECORD_FOUND; //Record doesnot exist 
}

extern RC updateRecord (RM_TableData *rel, Record *record)
{
		RM_RecordPoolInfo *record11=(RM_RecordPoolInfo *)rel->mgmtData;
		int status;
		if(record->id.page > 0 && record->id.page <=  ((BM_BufferPoolInfo *)(record11->bm)->mgmtData)->fh->totalNumPages)
		{
			BM_PageHandle *page = MAKE_PAGE_HANDLE();
			status = pinPage(record11->bm, page, record->id.page);
			if(status != RC_OK)
				return status;
			else
			{
				//check Primary Key with hash map
				Record *r11 = (Record *)malloc(sizeof(Record));
				RID rid;
				rid.page = record->id.page; //set page id
				rid.slot = record->id.slot; //set slot id
				status = getRecord (rel, rid, r11); //fetch data from record
				Value *Vall, *Valr;
				char *Serl, *Serr;
				status = getAttr (r11, rel->schema, rel->schema->keyAttrs[0], &Vall); // get the primary key attribute of the current location
				status = getAttr (record, rel->schema, rel->schema->keyAttrs[0], &Valr); // get the primary key attribute of the record to insert
				Serl = serializeValue(Vall); // serialize
				Serr = serializeValue(Valr); // serialize
				if(strcmp(Serl, Serr) != 0)
				{
					if(strncmp(r11->data, "deleted:", 7) == 0)
						return RC_RM_UPDATE_ON_DELETE_RECORD_ERROR;
					
					
					RM_RecordPoolInfo *recordmanage=(RM_RecordPoolInfo *)rel->mgmtData;
					int vp,spo=2;
					Value *val;
					char *seri_val;
					recordmanage->currentNode = recordmanage->arraykeys;
					status = getAttr (record, rel->schema, rel->schema->keyAttrs[0], &val);
					if(status == RC_OK)
					{
						seri_val = serializeValue(val);
						while(recordmanage->currentNode != NULL)
						{
							if(strcmp(seri_val, recordmanage->currentNode->data) == 0)
							{	
								vp= 1;
								goto abc;
							}
							recordmanage->currentNode = recordmanage->currentNode->pk_nkey;
						}
					}
					if(spo == 1)
					{
						check_cond((recordmanage->rp_epages[0] == 1),
						{
							recordmanage->arraykeys = (PrimaryKey_Record *)malloc(sizeof(PrimaryKey_Record));
							recordmanage->arraykeys->data = (char *)malloc(strlen(seri_val));
							strcpy(recordmanage->arraykeys->data, seri_val);
							recordmanage->arraykeys->pk_nkey = NULL;
							recordmanage->currentKey = recordmanage->arraykeys;
						},
						{
							recordmanage->currentKey->pk_nkey = (PrimaryKey_Record *)malloc(sizeof(PrimaryKey_Record));
							recordmanage->currentKey = recordmanage->currentKey->pk_nkey;
							recordmanage->currentKey->data = (char *)malloc(strlen(seri_val));
							strcpy(recordmanage->currentKey->data, seri_val);
							recordmanage->currentKey->pk_nkey = NULL;
						});
					}
						vp= 0;					
					
					abc:
					status=vp;
					if(status == 1)
						return RC_RM_PRIMARY_KEY_ALREADY_PRESENT_ERROR; //primary key exist
				}
				r11 = NULL;
				free(r11);
				memset(page->data, '\0', strlen(page->data));
				sprintf(page->data, "%s", record->data);
				status = markDirty(record11->bm, page);//mark the page isDirty
				if(status != RC_OK){}
				else
				{
					status = unpinPage(record11->bm, page);// unpin the page
					if(status != RC_OK){}
					else
					{
						status = forcePage(record11->bm, page);
						
						check_cond((status != RC_OK),{},
						{
							free(page); //free page
							return RC_OK;
						});
					}
				}
			}
			
		}
		return RC_RM_NO_RECORD_FOUND;
}

extern RC getRecord (RM_TableData *rel, RID id, Record *record)
{
		RM_RecordPoolInfo *record11=(RM_RecordPoolInfo *)rel->mgmtData;
		int status;
		if(id.page > 0 && id.page <=  ((BM_BufferPoolInfo *)(record11->bm)->mgmtData)->fh->totalNumPages)
		{
			BM_PageHandle *pg = MAKE_PAGE_HANDLE();//initialize the page
			status = pinPage(record11->bm, pg, id.page);//pin page			
			if(status != RC_OK)
				return status;
			else
			{   //store the record data and id
				record->id = id;
				record->data = pg->data;
				status = unpinPage(record11->bm, pg);//unpin page
				check_cond((status != RC_OK),{},
				{
					free(pg);//free page
					return RC_OK;
				});
			}
			
		}
		return RC_RM_NO_RECORD_FOUND;
}

extern RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond)
{
		if (rel == NULL)
			return RC_RM_NO_TABLE_INIT;
		//initialize the table record
		RecordM_PoolScan *sp;
		sp = (RecordM_PoolScan *)malloc(sizeof(RecordM_PoolScan));
		sp->cRecord=(Record *)malloc(sizeof(Record));
		sp->condition = cond;
		sp->pgcurrent = 1;
		scan->rel = rel;
		scan->mgmtData = sp;
		return RC_OK;
}

extern RC next (RM_ScanHandle *scan, Record *record)
{
		RecordM_PoolScan *sp=((RecordM_PoolScan *)scan->mgmtData);
		Value *out;
		RID rid;
		int status;
		rid.page = sp->pgcurrent;
		rid.slot = 0;
		if(sp-> condition !=NULL)
		{
			for(;rid.page > 0 && rid.page < ((BM_BufferPoolInfo *)(((RM_RecordPoolInfo *)(scan->rel)->mgmtData)->bm)->mgmtData)->fh->totalNumPages;)
			{        
				status = getRecord (scan->rel, rid, (sp->cRecord));
				status = evalExpr (sp->cRecord, scan->rel->schema,sp->condition, &out);
				check_cond((out->dt == DT_BOOL && out->v.boolV),
				{
					record->data = sp->cRecord->data;
					record->id = sp->cRecord->id;
					sp->pgcurrent=sp->pgcurrent+1 ;
					return RC_OK;
				},
				
				{
					sp->pgcurrent=sp->pgcurrent+1 ;					
					rid.page = sp->pgcurrent;
					rid.slot=0;
				});

			}
		}
		else	
		{
			for(;rid.page > 0 && rid.page < ((BM_BufferPoolInfo *)(((RM_RecordPoolInfo *)(scan->rel)->mgmtData)->bm)->mgmtData)->fh->totalNumPages;)
			{
				status = getRecord (scan->rel, rid, sp->cRecord);
				if(status!=RC_OK)
					return status;				
				else
				{
					record->data = sp->cRecord->data;
					record->id = sp->cRecord->id;
					sp->pgcurrent =sp->pgcurrent + 1;
				  
					rid.page = sp->pgcurrent;
					  rid.slot = 0;
					return RC_OK;
				}
				
			}
		}
		
		sp->pgcurrent= 1;
		return RC_RM_NO_MORE_TUPLES;
}


extern RC closeScan (RM_ScanHandle *scan)
{   
		RecordM_PoolScan *sp=((RecordM_PoolScan *)scan->mgmtData);
		//making the memory null and then freeing the memory
		sp->cRecord = NULL;    
		free(sp->cRecord);
		scan->mgmtData = NULL;
		free(scan->mgmtData);   	   
		scan = NULL;
		free(scan);
		return RC_OK;
}

extern int getRecordSize (Schema *schema)
{   int j=0, mem = 0;
			
			while(j < schema->numAttr)
			{
				if(schema->dataTypes[j] == DT_INT)
					mem += sizeof(int);
				else if(schema->dataTypes[j] == DT_FLOAT)
					mem += sizeof(float);
				else if(schema->dataTypes[j] == DT_BOOL)
					mem += sizeof(bool);
				else
					mem += schema->typeLength[j];
				j++;
			}
		int mem1 = mem/2;
		return mem1;
}

extern Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)
{   
   
		Schema *Schema1 = (Schema *) malloc(sizeof(Schema));
		Schema1->numAttr = numAttr;
		Schema1->attrNames = attrNames;
		Schema1->dataTypes = dataTypes;
		Schema1->typeLength = typeLength;
		Schema1->keyAttrs = keys;
		Schema1->keySize = keySize;
		return Schema1;
}

extern RC freeSchema (Schema *schema)
{   
		schema->numAttr = NULL;
		schema->attrNames = NULL;
		schema->dataTypes = NULL;
		schema->typeLength = NULL;
		schema->keyAttrs = NULL;
		schema->keySize = NULL;	  
		schema = NULL;
		free(schema);
		return RC_OK;
}

extern RC createRecord (Record **record, Schema *schema)
{
		int j=0, mem1 = 0;
			
			for(j = 0; j < schema->numAttr; j++)
			{
				if(schema->dataTypes[j] == DT_INT)
					mem1 += sizeof(int);
				else if(schema->dataTypes[j] == DT_FLOAT)
					mem1 += sizeof(float);
				else if(schema->dataTypes[j] == DT_BOOL)
					mem1 += sizeof(bool);
				else
					mem1 += schema->typeLength[j];
			}
		int mem = mem1;// get the required number of byte required for the record
		*record = (Record *)malloc(sizeof(Record));//allocating memory
		record[0]->data = (char *)malloc(mem + schema->numAttr + 1);
		int i=0;
		sprintf(record[0]->data, "%s", "(");
		while(i < schema->numAttr - 1)
		{
			strcat(record[0]->data,",");
			i++;
		}
		strcat(record[0]->data,")");		
		return RC_OK;
}

extern RC freeRecord (Record *record)
{
		record->data = NULL;
		free(record->data);
		record = NULL;
		free(record);
		return RC_OK;
}

extern RC getAttr (Record *record, Schema *schema, int attrNum, Value **value)
{
			
			int j=0, mem1 = 0;
			while(j < schema->numAttr)
			{
				if(schema->dataTypes[j] == DT_INT)
					mem1 += sizeof(int);
				else if(schema->dataTypes[j] == DT_FLOAT)
					mem1 += sizeof(float);
				else if(schema->dataTypes[j] == DT_BOOL)
					mem1 += sizeof(bool);
				else
					mem1 += schema->typeLength[j];
				j++;

			}
		
		int mem = mem1;
		int i;
		char *prev, *out,rd[1000];
		if(attrNum >= schema->numAttr) {return RC_RM_NO_MORE_TUPLES;}		
		
		else
		{
			prev = (char *)malloc(mem);
			out = (char *)malloc(schema->typeLength[attrNum]);
			if(attrNum == 0)
			{
				sprintf(prev, "%s", "(");
				sprintf(out, "%c", record->data[1]);
				for(i = 2; i < strlen(record->data); i++)
				{
					if(record->data[i] == ',')
						break;
					sprintf(rd, "%c", record->data[i]);
					strcat(out, rd);
				}
			}
			else if(attrNum > 0 && attrNum < schema->numAttr)
			{
				int rNC = attrNum, nC = 0;
				sprintf(prev, "%s", "(");
				
				for(i = 1; i < strlen(record->data); i++)
				{
					if(nC == rNC)
					{
						if(record->data[i] == ',' || record->data[i] == ')')
							break;
						sprintf(rd, "%c", record->data[i]);
						strcat(out, rd);
						continue;
					}
					if(record->data[i] == ',')
					{
						sprintf(out, "%c", record->data[++i]);
						nC++;
					}
					sprintf(rd, "%c", record->data[i]);
					strcat(prev, rd);
				}
			}
			Value *values = (Value*) malloc(sizeof(Value));
			if(schema->dataTypes[attrNum] == DT_INT)
				values->v.intV = atoi(out);
			else if(schema->dataTypes[attrNum] == DT_FLOAT)
				values->v.floatV = atof(out);
			else if(schema->dataTypes[attrNum] == DT_BOOL)
				values->v.boolV = (bool) *out;
			else
				values->v.stringV = out;
			values->dt = schema->dataTypes[attrNum];
			value[0] = values;
			prev = NULL;
			free(prev);
			out = NULL;
			free(out);
			return RC_OK;
		}
		
}

extern RC setAttr (Record *record, Schema *schema, int attrNum, Value *value)
{
		char rd[1000];
		char *prev, *pt;
		int i;
		int j=0, mem1 = 0;
			
			while(j < schema->numAttr)
			{
				if(schema->dataTypes[j] == DT_INT)
					mem1 += sizeof(int);
				else if(schema->dataTypes[j] == DT_FLOAT)
					mem1 += sizeof(float);
				else if(schema->dataTypes[j] == DT_BOOL)
					mem1 += sizeof(bool);
				else
					mem1 += schema->typeLength[j];
				j++;
			}
		int mem = mem1;
		prev = (char *)malloc(mem);
		pt = (char *)malloc(mem);
		if(attrNum >= schema->numAttr)
			return RC_RM_NO_MORE_TUPLES;		
		else
		{
			if(attrNum == 0)
			{
				sprintf(prev, "%s", "(");
				for(i = 1; i < strlen(record->data); i++)
				{
					if(record->data[i] == ',')
						break;
					sprintf(rd, "%c", record->data[i]);
					strcat(prev, rd);
				}
				sprintf(pt, "%s", ",");
				for( i = i + 1; i < strlen(record->data); i++)
				{
					sprintf(rd, "%c", record->data[i]);
					strcat(pt, rd);
				}
			}
			else if(attrNum > 0 && attrNum < schema->numAttr)
			{
				int rNC = attrNum, nC = 0;
				sprintf(prev, "%s", "(");
				for(i = 1; i < strlen(record->data); i++)            {
					if(nC > rNC)
						break;
					if(nC == rNC)
					{
						if(record->data[i] == ',')
							nC++;
						continue;
					}
					if(record->data[i] == ',')
						nC++;
					sprintf(rd, "%c", record->data[i]);
					strcat(prev, rd);
				}
				check_cond((attrNum != (schema->numAttr - 1)),
				{
					sprintf(pt, "%s", ",");
					for( ; i < strlen(record->data); i++)
					{
						sprintf(rd, "%c", record->data[i]);
						strcat(pt, rd);
					}
				},
				{sprintf(pt, "%s", ")");});
			}
			
			//Check type of attribute 
			if((*schema).dataTypes[attrNum] == DT_INT)       
				sprintf(rd, "%d", value->v.intV);
			else if((*schema).dataTypes[attrNum] == DT_FLOAT)		  
				sprintf(rd, "%f", value->v.floatV);
			else if((*schema).dataTypes[attrNum] == DT_BOOL)		  
				sprintf(rd, "%d", value->v.boolV);
			else
				strcpy(rd, value->v.stringV);					  
			strcpy(record->data, prev);
			strcat(record->data, rd);
			strcat(record->data, pt);
			prev = NULL;
			pt = NULL;
			free(prev);
			free(pt);
			return RC_OK;
		}
		
}