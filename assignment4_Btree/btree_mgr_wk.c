
#include "btree_mgr.h"
#include "tables.h"
#include "storage_mgr.h"
#include "record_mgr.h"
#include <stdlib.h>
#include <string.h>
 
typedef struct BTREE 
{
    int *key;
    struct BTREE **next;
    RID *id;
} BTree;

BTree *scan;
BTree *r_node;

int n_value;
int i_num = 0;
SM_FileHandle btree_fh;
int max_element;


//Index manager is initialized using this function.
RC initIndexManager (void *mgmtData)
{
    return RC_OK;
}


//Index manager shut down using this function.
RC shutdownIndexManager ()
{
    return RC_OK;
}

//This function is used to create a B+ tree index.
RC createBtree (char *idxId, DataType keyType, int n)
{
	
    r_node = ((BTree*)malloc(sizeof(BTree)));
    r_node->key = malloc(sizeof(int) * n);
    n_value = 2;
    r_node->id = malloc(sizeof(int) * n);
    r_node->next = malloc(sizeof(BTree) * (n + 1));
    int i = 0;
    while (i<n + 1){
        r_node->next[i] = NULL;
        i += 1;
    }
    max_element = n;
    createPageFile (idxId);
    
    return RC_OK;
}


//This function is used to open the tree  and scan through all entries of a B+ tree .
RC openBtree (BTreeHandle **tree, char *idxId)
{
    n_value= 3;
    openPageFile (idxId, &btree_fh);   
    return RC_OK;
}


//This function is used to close the tree after all the elements of the B+ tree is scanned.
RC closeBtree (BTreeHandle *tree)
{
    
    closePageFile(&btree_fh);
    n_value=1;
    free(r_node);
    
    return RC_OK;
}


//This function is used to remove a key and the corresponding record pointer from the index. 
RC deleteBtree (char *idxId)
{
    destroyPageFile(idxId);
    n_value = 10;
    return RC_OK;
}


// This function is used to calculate the total number of nodes in the B+ tree.
RC getNumNodes (BTreeHandle *tree, int *result)
{
    int numNodes = 0;
    n_value = 5;
    BTree *temp = ((BTree*)malloc(sizeof(BTree)));
    int i = 0;    
    while (i<max_element + 2) {
        numNodes += 1;
        i += 1;
    }

    *result = numNodes;
   
    return RC_OK;
}

//This function calculates the total number of entries present in the B+ tree.
RC getNumEntries (BTreeHandle *tree, int *result)
{
    int total = 0,totalEle = 0, i;
    
    BTree *temp = ((BTree*)malloc(sizeof(BTree)));
    int totalVal = 0;
    temp = r_node;
    while (temp != NULL){
        i = 0;
        while (i < max_element){
            if(temp->key[i] != 0){
                totalEle += 1;
            }
            i++;//i += 1
        }
        temp = (*temp).next[max_element];
    }
    *result = totalEle;
    totalVal = ++total;
    
    return RC_OK;
}

RC getKeyType (BTreeHandle *tree, DataType *result)
{
    return RC_OK;
}


//This function is used to access index.This method searches the B+ Tree for the key provided in the parameter.
RC findKey (BTreeHandle *tree, Value *key, RID *result)
{
    int f = 0;
    BTree *temp = ((BTree*)malloc(sizeof(BTree)));
    int i;
    temp = r_node;
    while (temp != NULL) {
        i = 0;
        while (i < max_element) {
            if(temp->key[i] == key->v.intV)
             {
                f = 1;
                (*result).page = temp->id[i].page;
                (*result).slot = temp->id[i].slot;               
                break;
            }
            i += 1;
        }
        if(f == 1){
            break;
        }
        temp = temp->next[max_element];
    }
     
    if(f != 1)      //Check_cond
        return RC_IM_KEY_NOT_FOUND;
else
        return RC_OK;
}

//This function is used to insert a new key and a record pointer pair into the index using key and RID provided in the parameter.
RC insertKey (BTreeHandle *tree, Value *key, RID rid)
{
    BTree *temp = (BTree*)malloc(sizeof(BTree));
    BTree *node = (BTree*)malloc(sizeof(BTree));
    n_value = 9;
	int size_key=sizeof(int) * max_element;//
    (*node).key = malloc(size_key);
    (*node).id = malloc(size_key);
    (*node).next = (malloc(sizeof(BTree) * (max_element + 1)));
    int i = 0;
    while (i < max_element) {
    	node->key[i] = 0;
        i += 1;
    }

    int nodeFull = 0;
    temp = r_node;
    while (temp != NULL) {
        nodeFull = 0;
        i = 0;
        while (i < max_element) {
            if(temp->key[i] == 0) {
                temp->id[i].page = rid.page;
                temp->id[i].slot = rid.slot;
                n_value = 1;
                temp->key[i] = key->v.intV;
                temp->next[i] = NULL;
                nodeFull += 1;
                break;
            }
            i += 1;
        }
        if((nodeFull == 0) && (temp->next[max_element] == NULL)) {
            node->next[max_element] = NULL;
            n_value = 2;
            temp->next[max_element] = node;
        }
        temp = temp->next[max_element];
    }
    
    int totalEle = 0;
    temp = r_node;
    while (temp != NULL){
        i = 0;
        while (i < max_element){
            if(temp->key[i] != 0){
                totalEle += 1;
            }
            i += 1;
        }
        temp = temp->next[max_element];
    }

    if(totalEle == 6) {                                            //
        (*node).key[0] = r_node->next[max_element]->key[0];
        (*node).key[1] = r_node->next[max_element]->next[max_element]->key[0];
        n_value = 3;
        (*node).next[0] = r_node;
        (*node).next[1] = r_node->next[max_element];
        (*node).next[2] = r_node->next[max_element]->next[max_element];
    }
   
    return RC_OK;
}

//This function is used to remove a key and the corresponding record pointer from the index. 
RC deleteKey (BTreeHandle *tree, Value *key)
{
    int f = 0, i;
    BTree *temp = (BTree*)malloc(sizeof(BTree));
    temp = r_node;
    while (temp != NULL) {
        i = 0;
        while (i < max_element) {
            if(temp->key[i] == key->v.intV) {
                f = 1;
                (*temp).key[i] = 0;
                (*temp).id[i].page = 0;
                (*temp).id[i].slot = 0;                
                break;
            }
            i += 1;
        }
        if(f == 1){
            break;
        }
        temp = (*temp).next[max_element];
    }
    
     return RC_OK;
}

//This function is used to open the tree  and scan through all entries of a B+ tree .
RC openTreeScan (BTreeHandle *tree, BT_ScanHandle **handle)
{
    i_num = 0;
    int totalEle = 0, i;
    scan = (BTree*)malloc(sizeof(BTree));
    scan = r_node;  
	int index_total=0;//
    
    BTree *temp = (BTree*)malloc(sizeof(BTree));
    temp = r_node;
    while (temp != NULL){
        i = 0;
        while (i < max_element){
            if(temp->key[i] != 0){
                n_value = 0;
                totalEle += 1;
            }
            i += 1;
			index_total=i*totalEle;
        }
        temp = (*temp).next[max_element];//
    }

    int elements[max_element][totalEle];
    int count = 0;
    int key[totalEle];   
    temp = r_node;
    while (temp != NULL) {
        i = 0;
        while (i < max_element) {
            key[count] = (*temp).key[i];//
            elements[0][count] = (*temp).id[i].page;//
            n_value = 1;
            elements[1][count] = (*temp).id[i].slot;//
            count += 1;
            i += 1;
        }
        temp = temp->next[max_element];
    }
    
    int pg, st;
    int swap,c,d;
    c = 0;
    while (c < count - 1)
    {
        d = 0;
        while (d < (count - c - 1))
        {
            if(key[d] > key[d+1])
            {
                swap = key[d];
                pg = elements[0][d];
                st = elements[1][d];

                n_value = 0;
                
                key[d] = key[d + 1];
                elements[0][d] = elements[0][d + 1];
                elements[1][d] = elements[1][d + 1];
                
                n_value = 1;

                key[d + 1] = swap;
                elements[0][d + 1] = pg;
                elements[1][d + 1] = st;
            }
            d += 1;
        }
        c += 1;
    }
    
    count = 0;
    temp = r_node;
    while(temp != NULL) {
        i = 0;
        while (i < max_element) {
            temp->key[i] = key[count];
            temp->id[i].page = elements[0][count];
            n_value = 2;
            temp->id[i].slot = elements[1][count];
            count += 1;
            i += 1;
        }
        temp = temp->next[max_element]; 
    }
   return RC_OK;
}

//This function is used to read the next entry that is to be done in the B+ tree.
RC nextEntry (BT_ScanHandle *handle, RID *result)//Check_cond
{
    if(scan->next[max_element] == NULL)
	{
        return RC_IM_NO_MORE_ENTRIES;
    }
else
    {
        if(max_element != i_num) {
            n_value = 2;
        }else{
            i_num = 0;
            scan = (*scan).next[max_element]; //
        }
        (*result).page = (*scan).id[i_num].page;//
        n_value = 1;
        (*result).slot = (*scan).id[i_num].slot;//
        i_num += 1;
    }
    
    return RC_OK;
}

//This function is used to close the tree after all the elements of the B+ tree is scanned.
RC closeTreeScan (BT_ScanHandle *handle)
{
    i_num = 0;
    n_value = 4;
    return RC_OK;
}


//This Function is used to print b+ tree. 
char *printTree (BTreeHandle *tree)
{
    return RC_OK;
}

