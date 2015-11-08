/**********************************************************************
tests simple index insertion and scans. 
************************************************************************/
#include <stdio.h>
#include <time.h>
#include "am.h"
#include "testam.h"
#include "pf.h"

#define FILE1	"file1"
#define MAXRECS	50
#define MAX_FNAME_LENGTH 80	/* max length for file name */
#define PF_MAX_BUFS	20	/* max # of buffers */

/************************************************************
Open the File.
allocate as many pages in the file as the buffer
manager would allow, and write the page number
into the data.
then, close file.
******************************************************************/
writefile(fname,size_of_file)
char *fname;
int size_of_file;
{
srand(time(NULL));
int i,j,k=0;
int fd,pagenum;
int *buf;
int error;

	/* open file1, and allocate a few pages in there */
	if ((fd=PF_OpenFile(fname))<0){
		PF_PrintError("open file1");
		exit(1);
	}
	// printf("opened %s\n",fname);

	for (j = 0; j < size_of_file; ++j)
	{
		for (i=0; i < PF_MAX_BUFS; i++){
			if ((error=PF_AllocPage(fd,&pagenum,&buf))!= PFE_OK){
				PF_PrintError("first buffer\n");
				exit(1);
			}
			*((int *)buf) = rand();
			// printf("allocated page %d\n",pagenum);
		}

		if ((error=PF_AllocPage(fd,&pagenum,&buf))==PFE_OK){
			printf("too many buffers, and it's still OK\n");
			exit(1);
		}

		/* unfix these pages */
		for (i=0; i < PF_MAX_BUFS; i++){
			if ((error=PF_UnfixPage(fd,k,TRUE))!= PFE_OK){
				PF_PrintError("unfix buffer\n");
				printf("%d %d\n",i,error);
				exit(1);
			}
			k++;
		}
	}	

	/* close the file */
	if ((error=PF_CloseFile(fd))!= PFE_OK){
		PF_PrintError("close file1\n");
		exit(1);
	}

}

/**************************************************************
print the content of file
*************************************************************/
readfile(fname)
char *fname;
{
int error;
int *buf;
int pagenum;
int fd;

	// printf("opening %s\n",fname);
	if ((fd=PF_OpenFile(fname))<0){
		PF_PrintError("open file");
		exit(1);
	}
	printfile(fd);
	if ((error=PF_CloseFile(fd))!= PFE_OK){
		PF_PrintError("close file");
		exit(1);
	}
}

printfile(fd)
int fd;
{
int error;
int *buf;
int pagenum;

	// printf("reading file\n");
	pagenum = -1;
	while ((error=PF_GetNextPage(fd,&pagenum,&buf))== PFE_OK){
		// printf("got page %d, %d\n",pagenum,*buf);
		if ((error=PF_UnfixPage(fd,pagenum,FALSE))!= PFE_OK){
			PF_PrintError("unfix");
			exit(1);
		}
	}
	if (error != PFE_EOF){
		PF_PrintError("not eof\n");
		exit(1);
	}
	// printf("eof reached\n");

}

main()
{
	int error;
	// int i;
	// RecIdType recid;	/* record id */
	// char buf[NAMELENGTH]; /* buffer to store chars */

	/* init */
	// printf("initializing\n");
	PF_Init();

	/* create a few files */
	if ((error=PF_CreateFile(FILE1))!= PFE_OK){
		PF_PrintError("file1");
		exit(1);
	}

	writefile(FILE1,5000);
	readfile(FILE1);
	
	// int error;
	int fd0,fd1,fd2;
	int *buf;
	int pagenum;
	int id0,id1,id2; /* index descriptor */
	char *ch;
	int sd0,sd1,sd2; /* scan descriptors */
	char fnamebuf[MAX_FNAME_LENGTH];	/* file name buffer */
	int recnum;	/* record number */
	int numrec;		/* # of records retrieved*/
	clock_t t;
	t=clock();

	// printf("Top Down B+ Tree\n");
	if ((fd0=PF_OpenFile(FILE1))<0){
		PF_PrintError("open file1\n");
		exit(1);
	}
	// printf("opened %s\n",FILE1);

	/* create index on the both field of the record*/
	// printf("creating indices\n");
	AM_CreateIndex(RELNAME,RECVAL_INDEXNO,INT_TYPE,sizeof(int));

	/* open the index */
	// printf("opening indices\n");
	sprintf(fnamebuf,"%s.%d",RELNAME,RECVAL_INDEXNO);
	id0 = PF_OpenFile(fnamebuf);

	/* insert into index */
	// printf("inserting into index\n");

	recnum=0;
	PF_GetFirstPage(fd0,&pagenum,&buf);
	AM_InsertEntry(id0,INT_TYPE,sizeof(int),buf,IntToRecId(recnum));
	PF_UnfixPage(fd0,pagenum,TRUE);
	recnum++;
	while ((error=PF_GetNextPage(fd0,&pagenum,&buf))== PFE_OK){
		AM_InsertEntry(id0,INT_TYPE,sizeof(int),buf,IntToRecId(recnum));
		recnum++;
		if ((error=PF_UnfixPage(fd0,pagenum,FALSE))!= PFE_OK){
			PF_PrintError("unfix");
			exit(1);
		}
	}
	if (error != PFE_EOF){
		PF_PrintError("not eof\n");
		exit(1);
	}
	// printf("eof reached\n");
	t=clock()-t;
	double time_taken=((double)t)/CLOCKS_PER_SEC;
	// printf("Time taken (sec): %f\n",time_taken);
	printf("%f,",time_taken);
	int num_nodes = 0;
	int nodes = AM_GetNumOfNodes(id0,&num_nodes);
	// printf("Number of Nodes: %d\n",nodes);
	printf("%d,",nodes);

	/* Let's see if the insert works */
	// printf("opening index scan on integer\n");
	sd0 = AM_OpenIndexScan(id0,INT_TYPE,sizeof(int),EQ_OP,NULL);
	// printf("retrieving recid's from scan descriptor %d\n",sd0);
	numrec = 0;
	while((recnum=RecIdToInt(AM_FindNextEntry(sd0)))>= 0){
		// printf("%d\n",recnum);
		numrec++;
	}
	// printf("retrieved %d records\n",numrec);

	/* destroy everything */
	// printf("closing down\n");
	AM_CloseIndexScan(sd0);
	PF_CloseFile(id0);
	AM_DestroyIndex(RELNAME,RECVAL_INDEXNO);

	if ((error=PF_CloseFile(fd0))!= PFE_OK){
		PF_PrintError("close fd1");
		exit(1);
	}
	// printf("closed %s\n",FILE1);

	// printf("Top Down B+ Tree with sorted data\n");
	PF_MergeSort(FILE1);

	t=clock();
	if ((fd1=PF_OpenFile(FILE1))<0){
		PF_PrintError("open file1\n");
		exit(1);
	}
	// printf("opened %s\n",FILE1);

	/* create index on the both field of the record*/
	// printf("creating indices\n");
	AM_CreateIndex(RELNAME,RECVAL_INDEXNO,INT_TYPE,sizeof(int));

	/* open the index */
	// printf("opening indices\n");
	sprintf(fnamebuf,"%s.%d",RELNAME,RECVAL_INDEXNO);
	id1 = PF_OpenFile(fnamebuf);

	/* insert into index */
	// printf("inserting into index\n");

	recnum=0;
	PF_GetFirstPage(fd1,&pagenum,&buf);
	AM_InsertEntry(id1,INT_TYPE,sizeof(int),buf,IntToRecId(recnum));
	PF_UnfixPage(fd1,pagenum,TRUE);
	recnum++;
	while ((error=PF_GetNextPage(fd1,&pagenum,&buf))== PFE_OK){
		AM_InsertEntry(id1,INT_TYPE,sizeof(int),buf,IntToRecId(recnum));
		recnum++;
		if ((error=PF_UnfixPage(fd1,pagenum,FALSE))!= PFE_OK){
			PF_PrintError("unfix");
			exit(1);
		}
	}
	if (error != PFE_EOF){
		PF_PrintError("not eof\n");
		exit(1);
	}
	// printf("eof reached\n");
	t=clock()-t;
	time_taken=((double)t)/CLOCKS_PER_SEC;
	// printf("Time taken (sec): %f\n",time_taken);
	printf("%f,",time_taken);
	num_nodes = 0;
	nodes = AM_GetNumOfNodes(id1,&num_nodes);
	// printf("Number of Nodes: %d\n",nodes);
	printf("%d,",nodes);
	/* Let's see if the insert works */
	// printf("opening index scan on integer\n");
	sd1 = AM_OpenIndexScan(id1,INT_TYPE,sizeof(int),EQ_OP,NULL);
	// printf("retrieving recid's from scan descriptor %d\n",sd1);
	numrec = 0;
	while((recnum=RecIdToInt(AM_FindNextEntry(sd1)))>= 0){
		// printf("%d\n",recnum);
		numrec++;
	}
	// printf("retrieved %d records\n",numrec);


	/* destroy everything */
	// printf("closing down\n");
	AM_CloseIndexScan(sd1);
	PF_CloseFile(id1);
	AM_DestroyIndex(RELNAME,RECVAL_INDEXNO);

	if ((error=PF_CloseFile(fd1))!= PFE_OK){
		PF_PrintError("close fd1");
		exit(1);
	}
	// printf("closed %s\n",FILE1);

	// printf("Bottom UP with sorted data\n");
	t=clock();
	time_taken=((double)t)/CLOCKS_PER_SEC;

	int x=BottomUpBulkLoad(FILE1,RELNAME,RECVAL_INDEXNO,INT_TYPE,sizeof(int));
	t=clock()-t;
	time_taken=((double)t)/CLOCKS_PER_SEC;
	// printf("Time taken (sec): %f\n",time_taken);
	printf("%f,",time_taken);
	sprintf(fnamebuf,"%s.%d",RELNAME,RECVAL_INDEXNO);
	id2 = PF_OpenFile(fnamebuf);
	num_nodes = 0;
	nodes = AM_GetNumOfNodes(id2,&num_nodes);
	// printf("Number of Nodes: %d\n",nodes);
	// printf("Height: %d\n",x);
	printf("%d\n",nodes);
	// printf("opening index scan on integer\n");
	sd2 = AM_OpenIndexScan(id2,INT_TYPE,sizeof(int),EQ_OP,NULL);
	// printf("retrieving recid's from scan descriptor %d\n",sd2);
	numrec = 0;
	while((recnum=RecIdToInt(AM_FindNextEntry(sd2)))>= 0){
		// printf("%d\n",recnum);
		numrec++;
	}
	// printf("retrieved %d records\n",numrec);

	/* destroy everything */
	// printf("closing down\n");
	AM_CloseIndexScan(sd2);
	PF_CloseFile(id2);
	AM_DestroyIndex(RELNAME,RECVAL_INDEXNO);
}
