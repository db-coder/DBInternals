/* testpf.c */
#include <stdio.h>
#include <time.h>
#include "pf.h"
#include "pftypes.h"

#define FILE1	"file1"
#define FILE2	"file2"
#define MAX_INT 2147483647

main()
{
int error;
int i;
int pagenum,*buf;
int *buf1,*buf2;
int fd1,fd2;



	/* create a few files */
	if ((error=PF_CreateFile(FILE1))!= PFE_OK){
		PF_PrintError("file1");
		exit(1);
	}
	printf("file1 created\n");

	if ((error=PF_CreateFile(FILE2))!= PFE_OK){
		PF_PrintError("file2");
		exit(1);
	}
	printf("file2 created\n");

	/* write to file1 */
	writefile(FILE1);

	/* print it out */
	readfile(FILE1);

	/* write to file2 */
	writefile(FILE2);

	/* print it out */
	readfile(FILE2);


	/* open both files */
	if ((fd1=PF_OpenFile(FILE1))<0){
		PF_PrintError("open file1\n");
		exit(1);
	}
	printf("opened file1\n");

	if ((fd2=PF_OpenFile(FILE2))<0 ){
		PF_PrintError("open file2\n");
		exit(1);
	}
	printf("opened file2\n");

	/* get rid of records  1, 3, 5, etc from file 1,
	and 0,2,4,6 from file2 */
	for (i=0; i < PF_MAX_BUFS; i++){
		if (i & 1){
			if ((error=PF_DisposePage(fd1,i))!= PFE_OK){
				PF_PrintError("dispose\n");
				exit(1);
			}
			printf("disposed %d of file1\n",i);
		}
		else {
			if ((error=PF_DisposePage(fd2,i))!= PFE_OK){
				PF_PrintError("dispose\n");
				exit(1);
			}
			printf("disposed %d of file2\n",i);
		}
	}

	if ((error=PF_CloseFile(fd1))!= PFE_OK){
		PF_PrintError("close fd1");
		exit(1);
	}
	printf("closed file1\n");

	if ((error=PF_CloseFile(fd2))!= PFE_OK){
		PF_PrintError("close fd2");
		exit(1);
	}
	printf("closed file2\n");
	/* print the files */
	readfile(FILE1);
	readfile(FILE2);


	/* destroy the two files */
	if ((error=PF_DestroyFile(FILE1))!= PFE_OK){
		PF_PrintError("destroy file1");
		exit(1);
	}
	if ((error=PF_DestroyFile(FILE2))!= PFE_OK){
		PF_PrintError("destroy file2");
		exit(1);
	}

	/* create them again */
	if ((fd1=PF_CreateFile(FILE1))< 0){
		PF_PrintError("create file1");
		exit(1);
	}
	printf("file1 created\n");

	if ((fd2=PF_CreateFile(FILE2))< 0){
		PF_PrintError("create file2");
		exit(1);
	}
	printf("file2 created\n");

	/* put stuff into the two files */
	writefile(FILE1);
	writefile(FILE2);

	/* Open the files, and see how the buffer manager
	handles more insertions, and deletions */
	/* open both files */
	if ((fd1=PF_OpenFile(FILE1))<0){
		PF_PrintError("open file1\n");
		exit(1);
	}
	printf("opened file1\n");

	if ((fd2=PF_OpenFile(FILE2))<0 ){
		PF_PrintError("open file2\n");
		exit(1);
	}
	printf("opened file2\n");
	srand(time(NULL));
	int k;
	for(k = 0; k < 19*19*2 ; k++){
		for (i=PF_MAX_BUFS; i < PF_MAX_BUFS*2 ; i++){
			if ((error=PF_AllocPage(fd2,&pagenum,&buf))!= PFE_OK){
				PF_PrintError("first buffer\n");
				exit(1);
			}
			*((int *)buf) = rand();
			if ((error=PF_UnfixPage(fd2,pagenum,TRUE))!= PFE_OK){
				PF_PrintError("unfix file1");
				exit(1);
			}
			printf("alloc %d file1\n",i,pagenum);

			if ((error=PF_AllocPage(fd1,&pagenum,&buf))!= PFE_OK){
				PF_PrintError("first buffer\n");
				exit(1);
			}
			*((int *)buf) = rand();
			if ((error=PF_UnfixPage(fd1,pagenum,TRUE))!= PFE_OK){
				PF_PrintError("dispose file1");
				exit(1);
			}
			printf("alloc %d file2\n",i,pagenum);
		}
	}

	for (i= PF_MAX_BUFS; i < PF_MAX_BUFS*2; i++){
		if (i & 1){
			if ((error=PF_DisposePage(fd1,i))!= PFE_OK){
				PF_PrintError("dispose fd1");
				exit(1);
			}
			printf("dispose fd1 page %d\n",i);
		}
		else {
			if ((error=PF_DisposePage(fd2,i))!= PFE_OK){
				PF_PrintError("dispose fd2");
				exit(1);
			}
			printf("dispose fd2 page %d\n",i);
		}
	}

	printf("getting file2\n");
	for (i=PF_MAX_BUFS; i < PF_MAX_BUFS*2; i++){
		if (i & 1){
			if ((error=PF_GetThisPage(fd2,i,&buf))!=PFE_OK){
				PF_PrintError("get this on fd2");
				exit(1);
			}
			printf("%d %d\n",i,*buf);
			if ((error=PF_UnfixPage(fd2,i,FALSE))!= PFE_OK){
				PF_PrintError("get this on fd2");
					exit(1);
			}
		}
	}

	printf("getting file1\n");
	for (i=PF_MAX_BUFS; i < PF_MAX_BUFS*2; i++){
		if (!(i & 1)){
			if ((error=PF_GetThisPage(fd1,i,&buf))!=PFE_OK){
				PF_PrintError("get this on fd2");
				exit(1);
			}
			printf("%d %d\n",i,*buf);
			if ((error=PF_UnfixPage(fd1,i,FALSE))!= PFE_OK){
				PF_PrintError("get this on fd2");
					exit(1);
			}
		}
	}

	/* print the files */
	printfile(fd2);

	printfile(fd1);

	/*put some more stuff into file1 */
	printf("putting stuff into holes in fd1\n"); 
	for (i=0; i < (PF_MAX_BUFS/2 -1); i++){
		if (PF_AllocPage(fd1,&pagenum,&buf)!= PFE_OK){
			PF_PrintError("PF_AllocPage");
			exit(1);
		}
		*buf =pagenum;
		if (PF_UnfixPage(fd1,pagenum,TRUE)!= PFE_OK){
			PF_PrintError("PF_UnfixPage");
			exit(1);
		}
	}

	printf("printing fd1");
	printfile(fd1);

	PF_CloseFile(fd1);
	printf("closed file1\n");

	PF_CloseFile(fd2);
	printf("closed file2\n");

	/* open file1 twice */
	if ((fd1=PF_OpenFile(FILE1))<0){
		PF_PrintError("open file1");
		exit(1);
	}
	printf("opened file1\n");

	/* try to destroy it while it's still open*/
	error=PF_DestroyFile(FILE1);
	PF_PrintError("destroy file1, should not succeed");


	/* get rid of some invalid page */
	error=PF_DisposePage(fd1,100);
	PF_PrintError("dispose page 100, should fail");


	/* get a valid page, and try to dispose it without unfixing.*/
	if ((error=PF_GetThisPage(fd1,1,&buf))!=PFE_OK){
		PF_PrintError("get this on fd2");
		exit(1);
	}
	printf("got page%d\n",*buf);
	error=PF_DisposePage(fd1,1);
	PF_PrintError("dispose page1, should fail");

	/* Now unfix it */
	if ((error=PF_UnfixPage(fd1,1,FALSE))!= PFE_OK){
		PF_PrintError("get this on fd2");
			exit(1);
	}

	error=PF_UnfixPage(fd1,1,FALSE);
	PF_PrintError("unfix fd1 again, should fail");

	if ((fd2=PF_OpenFile(FILE1))<0 ){
		PF_PrintError("open file1 again");
		exit(1);
	}
	printf("opened file1 again\n");

	printfile(fd1);

	printfile(fd2);

	if (PF_CloseFile(fd1) != PFE_OK){
		PF_PrintError("close fd1");
		exit(1);
	}

	if (PF_CloseFile(fd2)!= PFE_OK){
		PF_PrintError("close fd2");
		exit(1);
	}

	/* print the buffer */
	printf("buffer:\n");
	PFbufPrint();

	/* print the hash table */
	printf("hash table:\n");
	PFhashPrint();

	readfile(FILE1);
	mergesort(FILE1);
	readfile(FILE1);
	// readfile("temp");
}


/************************************************************
Open the File.
allocate as many pages in the file as the buffer
manager would allow, and write the page number
into the data.
then, close file.
******************************************************************/
writefile(fname)
char *fname;
{
srand(time(NULL));
int i;
int fd,pagenum;
int *buf;
int error;

	/* open file1, and allocate a few pages in there */
	if ((fd=PF_OpenFile(fname))<0){
		PF_PrintError("open file1");
		exit(1);
	}
	printf("opened %s\n",fname);

	for (i=0; i < PF_MAX_BUFS; i++){
		if ((error=PF_AllocPage(fd,&pagenum,&buf))!= PFE_OK){
			PF_PrintError("first buffer\n");
			exit(1);
		}
		*((int *)buf) = rand();
		printf("allocated page %d\n",pagenum);
	}

	if ((error=PF_AllocPage(fd,&pagenum,&buf))==PFE_OK){
		printf("too many buffers, and it's still OK\n");
		exit(1);
	}

	/* unfix these pages */
	for (i=0; i < PF_MAX_BUFS; i++){
		if ((error=PF_UnfixPage(fd,i,TRUE))!= PFE_OK){
			PF_PrintError("unfix buffer\n");
			exit(1);
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

	printf("opening %s\n",fname);
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

	printf("reading file\n");
	pagenum = -1;
	while ((error=PF_GetNextPage(fd,&pagenum,&buf))== PFE_OK){
		printf("got page %d, %d\n",pagenum,*buf);
		if ((error=PF_UnfixPage(fd,pagenum,FALSE))!= PFE_OK){
			PF_PrintError("unfix");
			exit(1);
		}
	}
	if (error != PFE_EOF){
		PF_PrintError("not eof\n");
		exit(1);
	}
	printf("eof reached\n");

}

mergesort(fname)
char* fname;
{
	int i;
	int fd,fdw,pagenum,runsize,wpagenum;
	int *buf;
	int error;
	int top[PF_MAX_BUFS-1];
	int page[PF_MAX_BUFS-1];
	int min_elem, curr_elem, min_i;
	int stop_i = 0;
	int swap = TRUE;
	int filesize;
	runsize = 1;


	// instead of setting this as -1, use get first page.
	pagenum = -1;
	wpagenum = -1;

	// Create another file with as many pages as the current one. It will be used as a buffer to store the partially sorted data.
	if ((error=PF_CreateFile("temp"))!= PFE_OK){
		PF_PrintError("file1");
		exit(1);
	}
	if ((fd=PF_OpenFile(fname))<0){
		PF_PrintError("open file");
		exit(1);
	}
	
	if ((fdw=PF_OpenFile("temp"))<0){
		PF_PrintError("open file");
		exit(1);
	}
	// change this to load multiple pages and write multiple pages at once.
	while ((error=PF_GetNextPage(fd,&pagenum,&buf))== PFE_OK){
		int x = *buf;
		if ((error=PF_AllocPage(fdw,&wpagenum,&buf))!= PFE_OK){
			PF_PrintError("The End1\n");
			exit(1);
		}
		*((int *)buf) = x;
		if ((error=PF_UnfixPage(fdw,wpagenum,TRUE))!= PFE_OK){
			PF_PrintError("unfix buffer\n");
			exit(1);
		}
		if ((error=PF_UnfixPage(fd,pagenum,FALSE))!= PFE_OK){
			PF_PrintError("unfix buffer\n");
			exit(1);
		}		
	}
	if ((error=PF_CloseFile(fd))!= PFE_OK){
		PF_PrintError("close file");
		exit(1);
	}
	if ((error=PF_DestroyFile(fname))!= PFE_OK){
		PF_PrintError("destroy fname");
		exit(1);
	}
	if ((error=PF_CreateFile(fname))!= PFE_OK){
		PF_PrintError("file1");
		exit(1);
	}
	if ((fd=PF_OpenFile(fname))<0){
		PF_PrintError("open file");
		exit(1);
	}
	pagenum = -1;
	wpagenum = -1;
	while ((error=PF_GetNextPage(fdw,&wpagenum,&buf))== PFE_OK){
		int x = *buf;
		if ((error=PF_AllocPage(fd,&pagenum,&buf))!= PFE_OK){
			PF_PrintError("Error Allocating Same page\n");
			exit(1);
		}
		*((int *)buf) = x;
		if ((error=PF_UnfixPage(fdw,wpagenum,FALSE))!= PFE_OK){
			PF_PrintError("unfix buffer\n");
			exit(1);
		}	
		if ((error=PF_UnfixPage(fd,pagenum,TRUE))!= PFE_OK){
			PF_PrintError("unfix buffer\n");
			exit(1);
		}		
	}	
	filesize = pagenum + 1;

	if ((error=PF_CloseFile(fd))!= PFE_OK){
		PF_PrintError("close file");
		exit(1);
	}
	if ((error=PF_CloseFile(fdw))!= PFE_OK){
		PF_PrintError("close file");
		exit(1);
	}
	// loop to create runs and merge them
	while(filesize > runsize){
		// printf("Read before a loop\n");
		// readfile(fname);
		// readfile("temp");
		// // Keep swapping between the original file and the temp file, to store the sorted data in that run.
		if(swap){
			if ((fd=PF_OpenFile(fname))<0){
				PF_PrintError("open file");
				exit(1);
			}
			
			if ((fdw=PF_OpenFile("temp"))<0){
				PF_PrintError("open file");
				exit(1);
			}
		}
		else{
			if ((fd=PF_OpenFile("temp"))<0){
				PF_PrintError("open file");
				exit(1);
			}
			
			if ((fdw=PF_OpenFile(fname))<0){
				PF_PrintError("open file");
				exit(1);
			}
		}

		pagenum = 0;
		wpagenum = -1;
		if(swap) swap = FALSE;
		else swap = TRUE;

		while (TRUE){
			stop_i = PF_MAX_BUFS + 1;
			for (i=0; i < PF_MAX_BUFS-1; i++){
				page[i] = runsize;
				top[i] = pagenum;
				if(pagenum >= filesize){
					stop_i = i - 1;
					if(i != 0) page[i-1] = filesize - top[i-1];
					break;
				}
				pagenum += runsize;
				// printf("%d %d\n",i , top[i]);
			}
			if(stop_i == -1){
				// printf("stopped because of no more data");
				break;
			}
			// printf("Runsize : %d\n",runsize);
			int iters = (stop_i)*runsize + page[stop_i];
			// printf("Iters, stop_i : %d %d\n",iters,stop_i);
			if(iters > (PF_MAX_BUFS-1)*runsize) iters = (PF_MAX_BUFS-1)*runsize;
			// printf("Iters, pageval : %d %d\n",iters,page[2]);
			int j;
			for(j = 0; j < iters; j++){
				min_elem = MAX_INT;
				min_i = -1;
				int runs = PF_MAX_BUFS - 1; // one is used for write
				if(stop_i < runs) runs = stop_i + 1;
				for (i=0; i < runs; i++){
					if(page[i] == 0) continue;
					// printf("%d\n",top[i]);
					if ((error=PF_GetThisPage(fd,top[i],&buf))!= PFE_OK){
						PF_PrintError("The End3\n");
						exit(1);
					}
					curr_elem = *buf;
					if(curr_elem < min_elem){
						min_i = i;
						min_elem = curr_elem;
					}
					if ((error=PF_UnfixPage(fd,top[i],FALSE))!= PFE_OK){
						PF_PrintError("unfix buffer\n");
						exit(1);
					}
				}

				if ((error=PF_GetNextPage(fdw,&wpagenum,&buf))!= PFE_OK){
					PF_PrintError("first buffer\n");
					exit(1);
				}
				*((int *)buf) = min_elem;
				page[min_i] -= 1;
				top[min_i] += 1;
				if ((error=PF_UnfixPage(fdw,wpagenum,TRUE))!= PFE_OK){
					// printf("%d\n",wpagenum);	
					PF_PrintError("unfix buffer\n");
					exit(1);
				}
			}
		}
		runsize *= (PF_MAX_BUFS-1);
		if ((error=PF_CloseFile(fd))!= PFE_OK){
			PF_PrintError("close file");
			exit(1);
		}
		if ((error=PF_CloseFile(fdw))!= PFE_OK){
			PF_PrintError("close file");
			exit(1);
		}

	}
	// if swap is false, write back to fname, else it is already there.
	if(swap == FALSE){
		pagenum = -1;
		wpagenum = -1;
		int tempbuf;

		if ((fd=PF_OpenFile(fname))<0){
			PF_PrintError("open file");
			exit(1);
		}
		
		if ((fdw=PF_OpenFile("temp"))<0){
			PF_PrintError("open file");
			exit(1);
		}
		// read all into buffer and write instead of one by one;
		while ((error=PF_GetNextPage(fdw,&wpagenum,&buf))== PFE_OK){
			tempbuf = *buf;
			if ((error=PF_GetNextPage(fd,&pagenum,&buf))!= PFE_OK){
				PF_PrintError("The End4\n");
				exit(1);
			}
			*((int *)buf) = tempbuf;
			if ((error=PF_UnfixPage(fd,pagenum,TRUE))!= PFE_OK){
				PF_PrintError("unfix");
				exit(1);
			}
			if ((error=PF_UnfixPage(fdw,wpagenum,FALSE))!= PFE_OK){
				PF_PrintError("unfix");
				exit(1);
			}
		}
		if ((error=PF_CloseFile(fd))!= PFE_OK){
			PF_PrintError("close file");
			exit(1);
		}
		if ((error=PF_CloseFile(fdw))!= PFE_OK){
			PF_PrintError("close file");
			exit(1);
		}
	}
	if ((error=PF_DestroyFile("temp"))!= PFE_OK){
		PF_PrintError("destroy temp");
		exit(1);
	}
	
}