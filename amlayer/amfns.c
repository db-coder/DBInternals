# include <stdio.h>
# include <math.h>
# include "pf.h"
# include "am.h"

int MAXKEYS;


BottomUpBulkLoad(sourceFile,indexFile,indexNo,attrType,attrLength)
char *sourceFile;/* Name of source file which contains the sorted values */
char *indexFile;/* Name of file which would contain the index */
int indexNo;/*number of this index for file */
char attrType;/* 'c' for char ,'i' for int ,'f' for float */
int attrLength; /* 4 for 'i' or 'f', 1-255 for 'c' */

{
	//TODO Declare variables
	char indexfName[AM_MAX_FNAME_LENGTH]; /* String to store the indexed
					files name with extension           */
	int inFileDesc; /* file Descriptor */
	int outFileDesc; /* file Descriptor */
	int errVal;
	int *inPageNum,InPageNum; inPageNum = &InPageNum;
	int *outPageNum,OutPageNum; outPageNum = &OutPageNum;
	int leafCount = 0;
	int height;
	int* pageNumbers;
	int firstPage;

	MAXKEYS = (PF_PAGE_SIZE - AM_sint - AM_si)/(AM_si + attrLength);

	/* Check the parameters */
	if ((attrType != 'c') && (attrType != 'f') && (attrType != 'i'))
	{
	AM_Errno = AME_INVALIDATTRTYPE;
	return(AME_INVALIDATTRTYPE);
	}

	if ((attrLength < 1) || (attrLength > 255))
	{
	AM_Errno = AME_INVALIDATTRLENGTH;
	return(AME_INVALIDATTRLENGTH);
	}

	if (attrLength != 4)
	if (attrType !='c')
	{
	AM_Errno = AME_INVALIDATTRLENGTH;
	return(AME_INVALIDATTRLENGTH);
	}


	char *pageBuf; /* buffer for holding a page */
	int pageNum; /* page number of the root page(also the first page) */

	/* Get the filename with extension and create a paged file by that name*/
	sprintf(indexfName,"%s.%d",indexFile,indexNo);
	// printf("%s.%d\n\n",indexFile,indexNo);
	errVal = PF_CreateFile(indexfName);
	AM_Check;


	/* open the input file */
	inFileDesc = PF_OpenFile(sourceFile);
	if (inFileDesc < 0)
	{
	AM_Errno = AME_PF;printf("%d",AM_Errno);
	return(AME_PF);
	}


	/* open the output file */
	outFileDesc = PF_OpenFile(indexfName);//printf("%d\n",inFileDesc);
	if (outFileDesc < 0)
	{
	AM_Errno = AME_PF;
	return(AME_PF);
	}



	// read elements from sourceFile and create leaf nodes

	char* buf;
	errVal=PF_GetFirstPage(inFileDesc,inPageNum,&buf);
	AM_Check;
	errVal = PF_UnfixPage(inFileDesc,*inPageNum,TRUE);
	AM_Check;

	//first leaf node
	int inFileEof = CreateLeaf(inFileDesc,outFileDesc,attrType,attrLength,inPageNum,outPageNum);
	AM_LeftPageNum = *outPageNum;
	int prevPageNum = *outPageNum;
	leafCount++;

	//rest of the leaf nodes
	while(!inFileEof){
		inFileEof = CreateLeaf(inFileDesc,outFileDesc,attrType,attrLength,inPageNum,outPageNum);
		changeNextPtrLeaf(outFileDesc,prevPageNum,*outPageNum);
		// printf("on to the next leaf\n");
		prevPageNum = *outPageNum;
		leafCount++;
	// printf("level count=%d\n",leafCount);
	}

	if(prevPageNum == AM_LeftPageNum){
		AM_RootPageNum = outPageNum;
		return 1;
	}

	// printf("level count=%d\n",leafCount);


	//TODO use leaf nodes to create internal nodes

	// int* firstNode;
	// int root_created = createInternalLayer(outFileDesc,attrType,attrLength,AM_LeftPageNum,firstNode,'l');


	// height = ceil(log10(leafCount)/log10(MAXKEYS));

	height = 1;
	while(leafCount>MAXKEYS){
		leafCount /= MAXKEYS;
		height++;
	}
	int root;
	// printf("%d\n", height);
	int globalLeafPtr = AM_LeftPageNum;
	int globalDone = FALSE;

	createIntLayer(outFileDesc, attrType, attrLength, height, &globalLeafPtr, &globalDone, &root);

	AM_RootPageNum = root;

	//TODO
	// /* initialise the root page */
	// AM_LeftPageNum ;
	// printf("%d\n",outFileDesc );
	// printf("end BootLoad\n");
	errVal = PF_CloseFile(inFileDesc);
	AM_Check;

	// printf("end BootLoad\n");
	errVal = PF_CloseFile(outFileDesc);
	AM_Check;


	// printf("end BootLoad\n");
	return height+1;

}


/*Creates a leaf nodes with values filled from file*/
CreateLeaf(inFileDesc,outFileDesc,attrType,attrLength,inPageNum,outPageNum)
int inFileDesc;//file from which we are reading values
int outFileDesc;//file on which we are creating the index
char attrType;/* 'c' for char ,'i' for int ,'f' for float */
int attrLength; /* 4 for 'i' or 'f', 1-255 for 'c' */
int* inPageNum;
int* outPageNum;

{
	// Declare varibles
	char *leafPageBuf; /* buffer for holding a page */
	char *inPageBuf; /* buffer for holding a page */
	int errVal;
	int maxKeys;/* Maximum keys that can be held on one internal page */
	AM_LEAFHEADER head,*header;
	int* curr_elem;
	int* prev_elem;
	int inserted;
	int index = 1;

	// printf("in create leaf \n");
	header = &head;
	/* allocate a new page for the root */
	errVal = PF_AllocPage(outFileDesc,outPageNum,&leafPageBuf);
	AM_Check;

// printf("OutPageNum = %d\n",*outPageNum);

// printf("in create leaf 1\n");
	/* initialise the header */
	header->pageType = 'l';
	header->nextLeafPage = AM_NULL_PAGE;
	header->recIdPtr = PF_PAGE_SIZE;
	header->keyPtr = AM_sl;
	header->freeListPtr = AM_NULL;
	header->numinfreeList = 0;
	header->attrLength = attrLength;
	header->numKeys = 0;
	/* the maximum keys in an internal node- has to be even always*/
	maxKeys = (PF_PAGE_SIZE - AM_sint - AM_si)/(AM_si + attrLength);
	if (( maxKeys % 2) != 0)
		header->maxKeys = maxKeys - 1;
	else
		header->maxKeys = maxKeys;
	/* copy the header onto the page */
	bcopy(header,leafPageBuf,AM_sl);

// printf("in create leaf 2\n");
	// printf(%d,);
	errVal=PF_GetThisPage(inFileDesc,*inPageNum,&inPageBuf);
	AM_Check;

	int x1=-50000;
	int x2=x1;

	x1 = *inPageNum;

	curr_elem = &x1;
	prev_elem = &x2;

	do{
		// printf("in do %d\n",*inPageNum);

		// printf("oye\n");
		AM_Check;
		if(x2==x1){
				// printf("inserting %d\n",x1);
				inserted=AM_InsertintoLeaf(leafPageBuf,attrLength,(char*)&x1,x1,index,TRUE);
				// printf(inserted)
			}
		else{
			// printf("inserting %d\n",x1);
		//  printf("index %d\n",index);
			inserted=AM_InsertintoLeaf(leafPageBuf,attrLength,(char*)&x1,x1,index,FALSE);
			index++;
		}
		// printf("x1 = %d\n",x1);
		if(inserted){
			errVal=PF_UnfixPage(inFileDesc,*inPageNum,TRUE);
			AM_Check;
				// printf("oyeLa\n");
			errVal = PF_GetNextPage(inFileDesc,inPageNum,&inPageBuf);
			x2=x1;
			x1 = *inPageNum;
		}

	}
	while(inserted && errVal != PFE_EOF );

	if(!inserted){
		errVal=PF_UnfixPage(inFileDesc,*inPageNum,TRUE);
	}

	// int x;
	// bcopy (leafPageBuf + AM_sl,(char *)&x,attrLength);
	//
	// printf("inside create leaf first value=%d\n",x);


	// printf("\n\n\n out of loop inserted = %d \n",inserted);
	errVal = PF_UnfixPage(outFileDesc,*outPageNum,TRUE);
	AM_Check;

	// printf("\n\nleaf created\n");


	if(!inserted)
		return 0;
	else
		return 1;
}

changeNextPtrLeaf(outFileDesc, prevPageNum, inPageNum)
int outFileDesc;
int prevPageNum;
int inPageNum;
{
	char* buf;
	AM_LEAFHEADER head; /* local header */
	AM_LEAFHEADER *header;

	header = &head;
	int errVal=PF_GetThisPage(outFileDesc,prevPageNum,&buf);
	AM_Check;
	bcopy(buf,header,AM_sl);
	header->nextLeafPage = inPageNum;
	bcopy(header,buf,AM_sl);

	errVal = PF_UnfixPage(outFileDesc,prevPageNum,TRUE);
	AM_Check;

}

createIntLayer(outFileDesc,attrType,attrLength,height, globalLeafPagePtr, globalDonePtr, curr_node, passValue)
int* outFileDesc;
char attrType;/* 'c' for char ,'i' for int ,'f' for float */
int attrLength; /* 4 for 'i' or 'f', 1-255 for 'c' */
int height; //height where we are working on
int* globalLeafPagePtr; //Which leaf node has been inserted
int* globalDonePtr; // Are all leaf page entries inserted?
int* curr_node; // returns root pointer
char* passValue; //return value of first element
{

	char value[AM_MAXATTRLENGTH];
	char dummyValue[AM_MAXATTRLENGTH];
	int pageNum,pageNum1,pageNum2;
	AM_INTHEADER newHead,*newHeader;
	int errVal;
	char *newPageBuf; /* buffer for holding the output interal page */
	char *buf; /* buffer for holding the input internal page */
	char *buf1; /* buffer for holding the input internal page */
	char *buf2; /* buffer for holding the input internal page */
	int maxKeys;

	if(height == 1){
		int k = CreateInt(outFileDesc,attrType,attrLength,globalLeafPagePtr,&curr_node,value);
		if(k==1){
			*globalDonePtr = 1;
		}
		else
			*globalDonePtr = 0;
		passValue = value;
		return 0;
	}

	int x = *(int*)value;

	// printf("%d = leaf ptr 1\n",*globalLeafPagePtr);
	createIntLayer(outFileDesc, attrType, attrLength, height-1, globalLeafPagePtr, globalDonePtr, &pageNum, &value);


	// printf("%d = done pointer\n",(*globalDonePtr));


	// printf("%d = leaf ptr 2\n",*globalLeafPagePtr);
	createIntLayer(outFileDesc, attrType, attrLength, height-1, globalLeafPagePtr, globalDonePtr, &pageNum1, &dummyValue);

	// printf("%d = done pointer\n",(*globalDonePtr));


	errVal=PF_AllocPage(outFileDesc,curr_node,&newPageBuf);
	AM_Check;
	newHeader = &newHead;

	/* fill the header */
	newHeader->pageType = 'i';
	newHeader->attrLength = attrLength;
	newHeader->maxKeys = MAXKEYS;
	newHeader->numKeys = 1;
	bcopy((char *)&pageNum,newPageBuf + AM_sint ,AM_si);
	bcopy(dummyValue,newPageBuf + AM_sint + AM_si ,attrLength);
	bcopy((char *)&pageNum1,newPageBuf + AM_sint + AM_si + attrLength,AM_si);
	bcopy(newHeader,newPageBuf,AM_sint);

	int offset = 1;
	int new_page_eof = 0;


	createIntLayer(outFileDesc, attrType, attrLength, height-1, globalLeafPagePtr, globalDonePtr, &pageNum2, &dummyValue);

	while( !(*globalDonePtr) && !new_page_eof){
		// printf("inside while numkeys = %d\n",newHeader->numKeys);
		if ((newHeader->numKeys) < (newHeader->maxKeys)){
		/* adds a key to an internal node */
		AM_AddtoIntPage(newPageBuf,dummyValue,pageNum2,newHeader,offset);
		// printf("added new pageNo to machaxx int node=%d\n",pageNum2);
		bcopy(newHeader,newPageBuf,AM_sint) ;
		}
		else{
			new_page_eof = 1 ;
			// printf("new page ends%d\n",newHeader->maxKeys);
		}

		// printf("(int)pageNum2!=(int)-1%d\n",(int)pageNum2!=(int)-1);
		// 	printf("(int)pageNum2=%d \n",(int)pageNum2);
		pageNum1 = pageNum2;
		// printf( "pageNum1=%d",pageNum1 );

		createIntLayer(outFileDesc, attrType, attrLength, height-1, globalLeafPagePtr, globalDonePtr, &pageNum2, &dummyValue);
		offset++;

	}

	errVal=PF_UnfixPage(outFileDesc,*curr_node,TRUE);
	AM_Check;

}

CreateInt(outFileDesc,attrType,attrLength,curr_daughter_node,curr_node,passValue)
int outFileDesc;//file on which we are creating the index
char attrType;/* 'c' for char ,'i' for int ,'f' for float */
int attrLength; /* 4 for 'i' or 'f', 1-255 for 'c' */
int* curr_daughter_node; //current daughter node accessed
int* curr_node;					//return page number of node created
char* passValue;
{

	// printf("inside createInt %d\n",*curr_daughter_node);

	// Declare varibles
	char *newPageBuf; /* buffer for holding the output interal page */
	char *buf; /* buffer for holding the input internal page */
	char *buf1; /* buffer for holding the input internal page */
	char *buf2; /* buffer for holding the input internal page */
	int errVal;
	int maxKeys;/* Maximum keys that can be held on one internal page */
	AM_LEAFHEADER leafHead,*leafHeader;
	AM_INTHEADER intHead,*intHeader;
	AM_INTHEADER newHead,*newHeader;
	int pageNum = *curr_daughter_node;
	int pageNum1;
	int pageNum2;
	char value[AM_MAXATTRLENGTH];
	int inserted;
	int index = 1;

	leafHeader = &leafHead;
	intHeader = &intHead;
	newHeader = &newHead;

	errVal=PF_GetThisPage(outFileDesc,pageNum,&buf1);
	AM_Check;


	int x;

	/* copy header from buffer */
	bcopy(buf1,leafHeader,AM_sl);
	pageNum1 = leafHeader->nextLeafPage;//Find next page
	maxKeys = leafHeader->maxKeys;
	bcopy (buf1 + AM_sl,(char*)&x,attrLength);

	// printf("inside create int value   =%d\n",x);

	errVal=PF_UnfixPage(outFileDesc,pageNum,FALSE);
	AM_Check;

	//get page 1 and 2
	errVal=PF_GetThisPage(outFileDesc,pageNum1,&buf2);
	AM_Check;

	/* copy header from buffer */
	bcopy(buf2,leafHeader,AM_sl);
	pageNum2 = leafHeader->nextLeafPage;
	bcopy(buf2 + AM_sl,passValue,attrLength);

	// printf("added new pageNo to int node=%d\n",pageNum1);

	errVal=PF_UnfixPage(outFileDesc,pageNum1,FALSE);
	AM_Check;

	errVal=PF_AllocPage(outFileDesc,curr_node,&newPageBuf);
	AM_Check;
	newHeader = &newHead;

	/* fill the header */
	newHeader->pageType = 'i';
	newHeader->attrLength = attrLength;
	newHeader->maxKeys = maxKeys;
	newHeader->numKeys = 1;
	bcopy((char *)&pageNum,newPageBuf + AM_sint ,AM_si);
	bcopy(value,newPageBuf + AM_sint + AM_si ,attrLength);
	bcopy((char *)&pageNum1,newPageBuf + AM_sint + AM_si + attrLength,AM_si);
	bcopy(newHeader,newPageBuf,AM_sint);

	int offset = 1;
	int new_page_eof = 0;

	while(pageNum2!=-1 && !new_page_eof){
		// printf("offset=%d",offset);
		if ((newHeader->numKeys) < (newHeader->maxKeys)){
		/* adds a key to an internal node */
		AM_AddtoIntPage(newPageBuf,value,pageNum2,newHeader,offset);
		// printf("added new pageNo to int node=%d\n",pageNum2);
		bcopy(newHeader,newPageBuf,AM_sint) ;
		}
		else{
			new_page_eof = 1 ;
			*curr_daughter_node = pageNum2;
			// printf("new page ends%d\n",newHeader->maxKeys);
		}

		// printf("(int)pageNum2!=(int)-1%d\n",(int)pageNum2!=(int)-1);
		// 	printf("(int)pageNum2=%d \n",(int)pageNum2);
		pageNum1 = pageNum2;
		// printf( "pageNum1=%d",pageNum1 );

		//get page 1 and 2
		int errVal=PF_GetThisPage(outFileDesc,pageNum1,&buf2);
		AM_Check;

		/* copy header from buffer */
		bcopy(buf2,leafHeader,AM_sl);
		pageNum2 = leafHeader->nextLeafPage;//Find next page
		bcopy(buf2 + AM_sl,value,attrLength);

		// printf("PageNum1=%d",pageNum1);
		// printf("PageNum2=%d\n",pageNum2);


		errVal=PF_UnfixPage(outFileDesc,pageNum1,FALSE);
		AM_Check;
		offset++;
	}

	errVal=PF_UnfixPage(outFileDesc,*curr_node,TRUE);
	AM_Check;

	if(pageNum2==AM_NULL_PAGE)
		return 1;
	else
		return 0;
}


/* Creates a secondary idex file called fileName.indexNo */
AM_CreateIndex(fileName,indexNo,attrType,attrLength)
char *fileName;/* Name of indexed file */
int indexNo;/*number of this index for file */
char attrType;/* 'c' for char ,'i' for int ,'f' for float */
int attrLength; /* 4 for 'i' or 'f', 1-255 for 'c' */


{
	char *pageBuf; /* buffer for holding a page */
	char indexfName[AM_MAX_FNAME_LENGTH]; /* String to store the indexed
					 files name with extension           */
	int pageNum; /* page number of the root page(also the first page) */
	int fileDesc; /* file Descriptor */
	int errVal;
	int maxKeys;/* Maximum keys that can be held on one internal page */
	AM_LEAFHEADER head,*header;

	/* Check the parameters */
	if ((attrType != 'c') && (attrType != 'f') && (attrType != 'i'))
		{
		 AM_Errno = AME_INVALIDATTRTYPE;
		 return(AME_INVALIDATTRTYPE);
                 }
        
	if ((attrLength < 1) || (attrLength > 255))
		{
		 AM_Errno = AME_INVALIDATTRLENGTH;
		 return(AME_INVALIDATTRLENGTH);
                }
	
	if (attrLength != 4)
		if (attrType !='c')
			{
			 AM_Errno = AME_INVALIDATTRLENGTH;
			 return(AME_INVALIDATTRLENGTH);
                        }
	
	header = &head;
	
	/* Get the filename with extension and create a paged file by that name*/
	sprintf(indexfName,"%s.%d",fileName,indexNo);
	errVal = PF_CreateFile(indexfName);
	AM_Check;

	/* open the new file */
	fileDesc = PF_OpenFile(indexfName);
	if (fileDesc < 0) 
	  {
	   AM_Errno = AME_PF;
	   return(AME_PF);
          }

	/* allocate a new page for the root */
	errVal = PF_AllocPage(fileDesc,&pageNum,&pageBuf);
	AM_Check;
	
	/* initialise the header */
	header->pageType = 'l';
	header->nextLeafPage = AM_NULL_PAGE;
	header->recIdPtr = PF_PAGE_SIZE;
	header->keyPtr = AM_sl;
	header->freeListPtr = AM_NULL;
	header->numinfreeList = 0;
	header->attrLength = attrLength;
	header->numKeys = 0;
	/* the maximum keys in an internal node- has to be even always*/
	maxKeys = (PF_PAGE_SIZE - AM_sint - AM_si)/(AM_si + attrLength);
	if (( maxKeys % 2) != 0) 
		header->maxKeys = maxKeys - 1;
	else 
		header->maxKeys = maxKeys;
	/* copy the header onto the page */
	bcopy(header,pageBuf,AM_sl);
	
	errVal = PF_UnfixPage(fileDesc,pageNum,TRUE);
	AM_Check;
	
	/* Close the file */
	errVal = PF_CloseFile(fileDesc);
	AM_Check;
	
	/* initialise the root page and the leftmost page numbers */
	AM_RootPageNum = pageNum;
	return(AME_OK);
}


/* Destroys the index fileName.indexNo */
AM_DestroyIndex(fileName,indexNo)
char *fileName;/* name of indexed file */
int indexNo; /* number of this index for file */

{
	char indexfName[AM_MAX_FNAME_LENGTH];
	int errVal;

	sprintf(indexfName,"%s.%d",fileName,indexNo);
	errVal = PF_DestroyFile(indexfName);
	AM_Check;
	return(AME_OK);
}


/* Deletes the recId from the list for value and deletes value if list
becomes empty */
AM_DeleteEntry(fileDesc,attrType,attrLength,value,recId)
int fileDesc; /* file Descriptor */
char attrType; /* 'c' , 'i' or 'f' */
int attrLength; /* 4 for 'i' or 'f' , 1-255 for 'c' */
char *value;/* Value of key whose corr recId is to be deleted */
int recId; /* id of the record to delete */

{
	char *pageBuf;/* buffer to hold the page */
	int pageNum; /* page Number of the page in buffer */
	int index;/* index where key is present */
	int status; /* whether key is in tree or not */
	short nextRec;/* contains the next record on the list */
	short oldhead; /* contains the old head of the list */
	short temp; 
	char *currRecPtr;/* pointer to the current record in the list */
	AM_LEAFHEADER head,*header;/* header of the page */
	int recSize; /* length of key,ptr pair for a leaf */
	int tempRec; /* holds the recId of the current record */
	int errVal; /* holds the return value of functions called within 
				                            this function */
	int i; /* loop index */


	/* check the parameters */
	if ((attrType != 'c') && (attrType != 'f') && (attrType != 'i'))
		{
		 AM_Errno = AME_INVALIDATTRTYPE;
		 return(AME_INVALIDATTRTYPE);
                }

	if (value == NULL) 
		{
		 AM_Errno = AME_INVALIDVALUE;
		 return(AME_INVALIDVALUE);
                }

	if (fileDesc < 0) 
		{
		 AM_Errno = AME_FD;
		 return(AME_FD);
                }

	/* initialise the header */
	header = &head;
	
	/* find the pagenumber and the index of the key to be deleted if it is
	there */
	status = AM_Search(fileDesc,attrType,attrLength,value,&pageNum,
			   &pageBuf,&index);
	
	/* check if return value is an error */
	if (status < 0) 
		{
		 AM_Errno = status;
		 return(status);
                }
	
	/* The key is not in the tree */
	if (status == AM_NOT_FOUND) 
		{
		 AM_Errno = AME_NOTFOUND;
		 return(AME_NOTFOUND);
                }
	
	bcopy(pageBuf,header,AM_sl);
	recSize = attrLength + AM_ss;
	currRecPtr = pageBuf + AM_sl + (index - 1)*recSize + attrLength;
	bcopy(currRecPtr,&nextRec,AM_ss);
	
	/* search the list for recId */
	while(nextRec != 0)
	{
		bcopy(pageBuf + nextRec,&tempRec,AM_si);
		
		/* found the recId to be deleted */
		if (recId == tempRec)
		{
			/* Delete recId */
			bcopy(pageBuf + nextRec + AM_si,currRecPtr,AM_ss);
			header->numinfreeList++;
			oldhead = header->freeListPtr;
			header->freeListPtr = nextRec;
			bcopy(&oldhead,pageBuf + nextRec + AM_si,AM_ss);
			break;
		}
		else 
	        {
			/* go over to the next item on the list */
			currRecPtr = pageBuf + nextRec + AM_si;
			bcopy(currRecPtr,&nextRec,AM_ss);
		}
	}
	
	/* if end of list reached then key not in tree */
	if (nextRec == AM_NULL)
		{
		 AM_Errno = AME_NOTFOUND;
		 return(AME_NOTFOUND);
                }
	
	/* check if list is empty */
	bcopy(pageBuf + AM_sl + (index - 1)*recSize + attrLength,&temp,AM_ss);
	if (temp == 0)
	{
		/* list is empty , so delete key from the list */
		for(i = index; i < (header->numKeys);i++)
			bcopy(pageBuf + AM_sl + i*recSize,pageBuf + AM_sl + 
				(i-1)*recSize,recSize);
		header->numKeys--;
		header->keyPtr = header->keyPtr - recSize;
	}
	
	/* copy the header onto the buffer */
	bcopy(header,pageBuf,AM_sl);
	
	errVal = PF_UnfixPage(fileDesc,pageNum,TRUE);
	
	/* empty the stack so that it is set for next amlayer call */
	AM_EmptyStack();
	  {
	   AM_Errno = AME_OK;
	   return(AME_OK);
          }
}

/* Inserts a value,recId pair into the tree */
AM_InsertEntry(fileDesc,attrType,attrLength,value,recId)
int fileDesc; /* file Descriptor */
char attrType; /* 'i' or 'c' or 'f' */
int attrLength; /* 4 for 'i' or 'f', 1-255 for 'c' */
char *value; /* value to be inserted */ 
int recId; /* recId to be inserted */

{
	char *pageBuf; /* buffer to hold page */
	int pageNum; /* page number of the page in buffer */
	int index; /* index where key can be found or can be inserted */
	int status; /* whether key is old or new */
	int inserted; /* Whether key has been inserted into the leaf or 
	                      splitting is needed */
	int addtoparent; /* Whether key has to be added to the parent */ 
	int errVal; /* return value of functions within this function */
	char key[AM_MAXATTRLENGTH]; /* holds the attribute to be passed 
						  back to the parent */

	
	/* check the parameters */
	if ((attrType != 'c') && (attrType != 'f') && (attrType != 'i'))
		{
		 AM_Errno = AME_INVALIDATTRTYPE;
		 return(AME_INVALIDATTRTYPE);
                }

	if (value == NULL) 
		{
		 AM_Errno = AME_INVALIDVALUE;
		 return(AME_INVALIDVALUE);
                }

	if (fileDesc < 0) 
		{
		 AM_Errno = AME_FD;
		 return(AME_FD);
                }
	
	
	/* Search the leaf for the key */
	status = AM_Search(fileDesc,attrType,attrLength,value,&pageNum,
			   &pageBuf,&index);


	
	/* check if there is an error */
	if (status < 0) 
	{ 
		AM_EmptyStack();
		AM_Errno = status;
		return(status);
	}
	
	/* Insert into leaf the key,recId pair */
	inserted = AM_InsertintoLeaf(pageBuf,attrLength,value,recId,index,
				     status);

	/* if key has been inserted then done */
	if (inserted == TRUE) 
	{
		errVal = PF_UnfixPage(fileDesc,pageNum,TRUE);
		AM_Check;
		AM_EmptyStack();
		return(AME_OK);
	}
	
	/* check if there is any error */
	if (inserted < 0) 
	{
		AM_EmptyStack();
		AM_Errno = inserted;
		return(inserted);
	}
	
	/* if not inserted then have to split */
	if (inserted == FALSE)
	{
		/* Split the leaf page */
		addtoparent = AM_SplitLeaf(fileDesc,pageBuf,&pageNum,
			     attrLength,recId,value, status,index,key);
		
		/* check for errors */
		if (addtoparent < 0) 
		{
			AM_EmptyStack();
			{
			 AM_Errno = addtoparent;
			 return(addtoparent);
                        }
		}
		
		/* if key has to be added to the parent */
		if (addtoparent == TRUE)
		{
			errVal = AM_AddtoParent(fileDesc,pageNum,key,attrLength);
			if (errVal < 0)
			{
				AM_EmptyStack();
				AM_Errno = errVal;
				return(errVal);
			}
		}
	}
	AM_EmptyStack();
	return(AME_OK);
}


/* error messages */
static char *AMerrormsg[] = {
"No error",
"Invalid Attribute Length",
"Key Not Found in Tree",
"PF error",
"Internal error - contact database manager immediately",
"Invalid scan Descriptor",
"Invalid operator to OpenIndexScan",
"Scan Over",
"Scan Table is full",
"Invalid Attribute Type",
"Invalid file Descriptor",
"Invalid value to Delete or Insert Entry"
};


AM_PrintError(s)
char *s;

{
   fprintf(stderr,"%s",s);
   fprintf(stderr,"%s",AMerrormsg[-1*AM_Errno]);
   if (AM_Errno == AME_PF)
      /* print the PF error message */
      PF_PrintError(" ");
   else 
     fprintf(stderr,"\n");
}

AM_GetNumOfNodes(fileDesc,num_nodes)
int fileDesc;//File Desc of the indexed file
int *num_nodes;
{
	int pageNum;
	char* pageBuf;
	
	int errVal =  PF_GetFirstPage(fileDesc,&pageNum,&pageBuf);
	int nodes = 0;
	int errval1 = 0;
	while(errval1 == 0){
	    nodes = nodes + 1;
		PF_UnfixPage(fileDesc,pageNum,FALSE);
		errval1 = PF_GetNextPage(fileDesc,&pageNum,&pageBuf);			
	}
	return nodes;
}
