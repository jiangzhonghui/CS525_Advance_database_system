CS 525----Assignment 4---B+ Tree Index------------


The objective of this assignmnet is to implement a disk-based B+-tree index structure, where the pages of the index are accessed through the Buffer Manager implemented in Assignment 2.

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

Run test_assign4_1 : ./test_assign4_1
Run test_expr      : ./test_expr


To remove object files, run make clean command

 ------B+-Tree Functions------
 
1.createBtree() - 
This function is used to create a new B+ tree index and allocates memory to all the elements of Btree struct.

2.openBtree() -
This function is used to open an existing B+ tree, which opens the page file specified by the input parameter 'idxId'.

3.closeBtree() -
This function is used to close an open B+ tree. Index manager ensures that all the modified or new pages of the index are flushed back to the disk, before the allocated memory is released.

4.deleteBtree() -
This function deletes a B tree index and its corresponding page file.

-------Functions for Accessing Info about the B+- Tree--------

5.getNumNodes() - 
This fuction is used to calculate the total number of nodes in the B+ tree.

6.getNumEntries() -
This function is used to return the number of enteries/keys present in the B+ tree

7.getKeyType() -
This function returns the datatype of the keys being stored in the B tree.

---------B+ Tree Index Access Functions---------

8.findKey() -
This function is used to search the B+ tree for the key specified in the input parameter 'key'. I fthe searched key does not exist, it return 'RC_IM_KEY_NOT_FOUND'.

9.insertKey() -
This function is used to insert a new key and a record pointer pair into the index, specified by the key and RID provided in the input parameter.

10.deleteKey() - 
This function is used to remove a key, specified the input parameter 'key' and the corresponding record pointer from the index.

11.openTreeScan() - 
It initializes the scan and scans through all the enteries of the B+ tree.

12.nextEntry() - 
This fuction is used to read the next entry in the B+ tree. 

13.closeTreeScan() - 
This function is used to close the tree after scanning through all the elements of the B+ tree.

------- Debugging/Test Functions -------

14.printTree() -
This functionis used to print the B+ tree for debugging.

-------- Index Manager Functions--------

15.initIndexManager() -
 This function initializes the Index manager.

16.shutdownIndexManager() - 
This function shuts down the Index manager and deallocates the resources. 
