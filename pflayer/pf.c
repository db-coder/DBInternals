/* pf.c: Paged File Interface Routines+ support routines */
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/file.h>
#include "pf.h"
#include "pftypes.h"

/* To keep system V and PC users happy */
#ifndef L_SET
#define L_SET 0
#endif

int PFerrno = PFE_OK;	/* last error message */

static PFftab_ele PFftab[PF_FTAB_SIZE]; /* table of opened files */

/* true if file descriptor fd is invaild */
#define PFinvalidFd(fd) ((fd) < 0 || (fd) >= PF_FTAB_SIZE \
				|| PFftab[fd].fname == NULL)

/* true if page number "pagenum" of file "fd" is invalid in the
sense that it's <0 or >= # of pages in the file */
#define PFinvalidPagenum(fd,pagenum) ((pagenum)<0 || (pagenum) >= \
				PFftab[fd].hdr.numpages)

#define MAX_INT 2147483647

extern char *malloc();

/****************** Internal Support Functions *****************************/
static char *savestr(str)
char *str;		/* string to be saved */
/****************************************************************************
SPECIFICATIONS:
	Allocate memory and save string pointed by str.
	Return a pointer to the saved string, or NULL if no memory.
*****************************************************************************/
{
char *s;

	if ((s=malloc(strlen(str)+1))!= NULL)
		strcpy(s,str);
	return(s);
}

static PFtabFindFname(fname)
char *fname;		/* file name to find */
/****************************************************************************
SPECIFICATIONS:
	Find the index to PFftab[] entry whose "fname" field is the
	same as "fname". 

AUTHOR: clc

RETURN VALUE:
	The desired index, or 
	-1	if not found

*****************************************************************************/
{
int i;

	for (i=0; i < PF_FTAB_SIZE; i++){
		if(PFftab[i].fname != NULL&& strcmp(PFftab[i].fname,fname) == 0)
			/* found it */
			return(i);
	}
	return(-1);
}

static PFftabFindFree()
/****************************************************************************
SPECIFICATIONS:
	Find a free entry in the open file table "PFtab", and return its
	index.

AUTHOR: clc

RETURN VALUE:
	If >=0, the index of the free entry.
	Otherwise, none can be found.

*****************************************************************************/
{
int i;

	for (i=0; i < PF_FTAB_SIZE; i++)
		if (PFftab[i].fname == NULL)
			return(i);
	return(-1);
}

PFreadfcn(fd,pagenum,buf)
int fd;	/* file descriptor */
int pagenum; /* page number */
PFfpage *buf;
/****************************************************************************
SPECIFICATIONS:
	Read the paged numbered "pagenum" from the file indexed by "fd"
	into the page buffer "buf".

AUTHOR: clc

RETURN VALUE:
	PFE_OK	if ok
	PF error code if not OK.
*****************************************************************************/
{
int error;

	/* seek to the appropriate place */
	if ((error=lseek(PFftab[fd].unixfd,pagenum*sizeof(PFfpage)+PF_HDR_SIZE,
				L_SET)) == -1){
		PFerrno = PFE_UNIX;
		return(PFerrno);
	}

	/* read the data */
	if((error=read(PFftab[fd].unixfd,(char *)buf,sizeof(PFfpage)))
			!=sizeof(PFfpage)){
		if (error <0)
			PFerrno = PFE_UNIX;
		else	PFerrno = PFE_INCOMPLETEREAD;
		return(PFerrno);
	}

	return(PFE_OK);
}

PFwritefcn(fd,pagenum,buf)
int fd;		/* file descriptor */
int pagenum;	/* page to read */
PFfpage *buf;	/* buffer where to read the page */
/****************************************************************************
SPECIFICATIONS:
	Write the page numbered "pagenum" from the buffer indexed
	by "buf" into the file indexed by "fd".

AUTHOR: clc

RETURN VALUE:
	PFE_OK	if ok.
	PF errod code if not OK.

*****************************************************************************/
{
int error;

	/* seek to the right place */
	if ((error=lseek(PFftab[fd].unixfd,pagenum*sizeof(PFfpage)+PF_HDR_SIZE,
				L_SET)) == -1){
		PFerrno = PFE_UNIX;
		return(PFerrno);
	}

	/* write out the page */
	if((error=write(PFftab[fd].unixfd,(char *)buf,sizeof(PFfpage)))
			!=sizeof(PFfpage)){
		if (error <0)
			PFerrno = PFE_UNIX;
		else	PFerrno = PFE_INCOMPLETEWRITE;
		return(PFerrno);
	}

	return(PFE_OK);

}


/************************* Interface Routines ****************************/

void PF_Init()
/****************************************************************************
SPECIFICATIONS:
	Initialize the PF interface. Must be the first function called
	in order to use the PF ADT.

AUTHOR: clc

RETURN VALUE: none

GLOBAL VARIABLES MODIFIED:
	PFftab
*****************************************************************************/
{
int i;
	/* init the hash table */
	PFhashInit();

	/* init the file table to be not used*/
	for (i=0; i < PF_FTAB_SIZE; i++){
		PFftab[i].fname = NULL;
	}
}

PF_CreateFile(fname)
char *fname;	/* name of file to create */
/****************************************************************************
SPECIFICATIONS:
	Create a paged file called "fname". The file should not have
	already existed before.

AUTHOR: clc

RETURN VALUE:
	PFE_OK	if OK
	PF error code if error.
*****************************************************************************/
{
int fd;	/* unix file descripotr */
PFhdr_str hdr;	/* file header */
int error;

	/* create file for exclusive use */
	if ((fd=open(fname,O_CREAT|O_EXCL|O_WRONLY,0664))<0){
		/* unix error on open */
		PFerrno = PFE_UNIX;
		return(PFE_UNIX);
	}

	/* write out the file header */
	hdr.firstfree = PF_PAGE_LIST_END;	/* no free pag yet */
	hdr.numpages = 0;
	if ((error=write(fd,(char *)&hdr,sizeof(hdr))) != sizeof(hdr)){
		/* error while writing. Abort everything. */
		if (error < 0)
			PFerrno = PFE_UNIX;
		else PFerrno = PFE_HDRWRITE;
		close(fd);
		unlink(fname);
		return(PFerrno);
	}

	if ((error=close(fd)) == -1){
		PFerrno = PFE_UNIX;
		return(PFerrno);
	}

	return(PFE_OK);
}


PF_DestroyFile(fname)
char *fname;		/* file name to destroy */
/****************************************************************************
SPECIFICATIONS:
	Destroy the paged file whose name is "fname". The file should
	exist, and should not be already open.

AUTHOR:
	clc

RETURN VALUE:
	PFE_OK 	if success
	PF error codes if error
*****************************************************************************/
{
int error;

	if (PFtabFindFname(fname)!= -1){
		/* file is open */
		PFerrno = PFE_FILEOPEN;
		return(PFerrno);
	}

	if ((error =unlink(fname))!= 0){
		/* unix error */
		PFerrno = PFE_UNIX;
		return(PFerrno);
	}

	/* success */
	return(PFE_OK);
}


PF_OpenFile(fname)
char *fname;		/* name of the file to open */
/****************************************************************************
SPECIFICATIONS:
	Open the paged file whose name is fname.  It is possible to open
	a file more than once. Warning: Openinging a file more than once for 
	write operations is not prevented. The possible consequence is
	the corruption of the file structure, which will crash
	the Paged File functions. On the other hand, opening a file
	more than once for reading is OK.

AUTHOR: clc

RETURN VALUE:
	The file descriptor, which is >= 0, if no error.
	PF error codes otherwise.

IMPLEMENTATION NOTES:
	A file opened more than once will have different file descriptors
	returned. Separate buffers are used.
*****************************************************************************/
{
int count;	/* # of bytes in read */
int fd; /* file descriptor */

	/* find a free entry in the file table */
	if ((fd=PFftabFindFree())< 0){
		/* file table full */
		PFerrno = PFE_FTABFULL;
		return(PFerrno);
	}

	/* open the file */
	if ((PFftab[fd].unixfd = open(fname,O_RDWR))< 0){
		/* can't open the file */
		PFerrno = PFE_UNIX;
		return(PFerrno);
	}

	/* Read the file header */
	if ((count=read(PFftab[fd].unixfd,(char *)&PFftab[fd].hdr,PF_HDR_SIZE))
				!= PF_HDR_SIZE){
		if (count < 0)
			/* unix error */
			PFerrno = PFE_UNIX;
		else	/* not enough bytes in file */
			PFerrno = PFE_HDRREAD;
		close(PFftab[fd].unixfd);
		return(PFerrno);
	}
	/* set file header to be not changed */
	PFftab[fd].hdrchanged = FALSE;

	/* save the file name */
	if ((PFftab[fd].fname = savestr(fname)) == NULL){
		/* no memory */
		close(PFftab[fd].unixfd);
		PFerrno = PFE_NOMEM;
		return(PFerrno);
	}

	return(fd);
}

PF_CloseFile(fd)
int fd;		/* file descriptor to close */
/****************************************************************************
SPECIFICATIONS:
	Close the file indexed by file descriptor fd. The file should have
	been opened with PFopen(). It is an error to close a file
	with pages still fixed in the buffer.

AUTHOR: clc

RETURN VALUE:
	PFE_OK	if OK
	PF error code if error.

*****************************************************************************/
{
int error;

	if (PFinvalidFd(fd)){
		/* invalid file descriptor */
		PFerrno = PFE_FD;
		return(PFerrno);
	}
	

	/* Flush all buffers for this file */
	if ( (error=PFbufReleaseFile(fd,PFwritefcn)) != PFE_OK)
		return(error);

	if (PFftab[fd].hdrchanged){
		/* write the header back to the file */
		/* First seek to the appropriate place */
		if ((error=lseek(PFftab[fd].unixfd,(unsigned)0,L_SET)) == -1){
			/* seek error */
			PFerrno = PFE_UNIX;
			return(PFerrno);
		}

		/* write header*/
		if((error=write(PFftab[fd].unixfd, (char *)&PFftab[fd].hdr,
				PF_HDR_SIZE))!=PF_HDR_SIZE){
			if (error <0)
				PFerrno = PFE_UNIX;
			else	PFerrno = PFE_HDRWRITE;
			return(PFerrno);
		}
		PFftab[fd].hdrchanged = FALSE;
	}


		
	/* close the file */
	if ((error=close(PFftab[fd].unixfd))== -1){
		PFerrno = PFE_UNIX;
		return(PFerrno);
	}

	/* free the file name space */
	free((char *)PFftab[fd].fname);
	PFftab[fd].fname = NULL;

	return(PFE_OK);
}


PF_GetFirstPage(fd,pagenum,pagebuf)
int fd;	/* file descriptor */
int *pagenum;	/* page number of first page */
char **pagebuf;	/* pointer to the pointer to buffer */
/****************************************************************************
SPECIFICATIONS:
	Read the first page into memory and set *pagebuf to point to it.
	Set *pagenum to the page number of the page read.
	The page read is fixed in the buffer until it is unixed with
	PFunfix().

AUTHOR: clc

RETURN VALUE:
	PFE_OK	if no error.
	PFE_EOF	if end of file reached.(meaning there is no first page. )
	other PF error code if other error.

*****************************************************************************/
{

	*pagenum = -1;
	return(PF_GetNextPage(fd,pagenum,pagebuf));
}


PF_GetNextPage(fd,pagenum,pagebuf)
int fd;	/* file descriptor of the file */
int *pagenum;	/* old page number on input, new page number on output */
char **pagebuf;	/* pointer to pointer to buffer of page data */
/****************************************************************************
SPECIFICATIONS:
	Read the next valid page after *pagenum, the current page number,
	and set *pagebuf to point to the page data. Set *pagenum
	to be the new page number. The new page is fixed in memory 
	until PFunfix() is called.
	Note that PF_GetNextPage() with *pagenum == -1 will return the 
	first valid page. PFgetFirst() is just a short hand for this.

AUTHOR: clc

RETURN VALUE:
	PFE_OK	if success
	PFE_EOF	if end of file reached without encountering
		any used page data. 
	PFE_INVALIDPAGE  if page number is invalid.
	other PF errors code for other error.

*****************************************************************************/
{
int temppage;	/* page number to scan for next valid page */
int error;	/* error code */
PFfpage *fpage;	/* pointer to file page */

	if (PFinvalidFd(fd)){
		PFerrno = PFE_FD;
		return(PFerrno);
	}


	if (*pagenum < -1 || *pagenum >= PFftab[fd].hdr.numpages){
		PFerrno = PFE_INVALIDPAGE;
		return(PFerrno);
	}

	/* scan the file until a valid used page is found */
	for (temppage= *pagenum+1;temppage<PFftab[fd].hdr.numpages;temppage++){
		if ( (error=PFbufGet(fd,temppage,&fpage,PFreadfcn,
					PFwritefcn))!= PFE_OK)
			return(error);
		else if (fpage->nextfree == PF_PAGE_USED){
			/* found a used page */
			*pagenum = temppage;
			*pagebuf = (char *)fpage->pagebuf;
			return(PFE_OK);
		}

		/* page is free, unfix it */
		if ((error=PFbufUnfix(fd,temppage,FALSE))!= PFE_OK)
			return(error);
	}

	/* No valid used page found */
	PFerrno = PFE_EOF;
	return(PFerrno);

}

PF_GetThisPage(fd,pagenum,pagebuf)
int fd;		/* file descriptor */
int pagenum;	/* page number to read */
char **pagebuf;	/* pointer to pointer to page data */
/****************************************************************************
SPECIFICATIONS:
	Read the page specifeid by "pagenum" and set *pagebuf to point
	to the page data. The page number should be valid.

AUTHOR: clc

RETURN VALUE:
	PFE_OK	if no error.
	PFE_INVALIDPAGE if invalid page number is specified.
	PFE_PAGEFIXED if page already fixed in memory. In this case,
		*pagebuf  is still set to point to the buffer that contains
		the page data.
	other PF error codes if other error encountered.
*****************************************************************************/
{
int error;
PFfpage *fpage;

	if (PFinvalidFd(fd)){
		PFerrno = PFE_FD;
		return(PFerrno);
	}

	if (PFinvalidPagenum(fd,pagenum)){
		PFerrno = PFE_INVALIDPAGE;
		return(PFerrno);
	}

	if ( (error=PFbufGet(fd,pagenum,&fpage,PFreadfcn,PFwritefcn))!= PFE_OK){
		if (error== PFE_PAGEFIXED)
			*pagebuf = fpage->pagebuf;
		return(error);
	}

	if (fpage->nextfree == PF_PAGE_USED){
		/* page is used*/
		*pagebuf = (char *)fpage->pagebuf;
		return(PFE_OK);
	}
	else {
		/* invalid page */
		if (PFbufUnfix(fd,pagenum,FALSE)!= PFE_OK){
			printf("internal error:PFgetThis()\n");
			exit(1);
		}
		PFerrno = PFE_INVALIDPAGE;
		return(PFerrno);
	}
}

PF_AllocPage(fd,pagenum,pagebuf)
int fd;		/* file descriptor */
int *pagenum;	/* page number */
char **pagebuf;	/* pointer to pointer to page buffer*/
/****************************************************************************
SPECIFICATIONS:
	Allocate a new, empty page for file "fd".
	set *pagenum to the new page number. 
	Set *pagebuf to point to the buffer for that page.
	The page allocated is fixed in the buffer.

AUTHOR: clc

RETURN VALUE:
	PFE_OK	if ok
	PF error codes if not ok.

*****************************************************************************/
{
PFfpage *fpage;	/* pointer to file page */
int error;

	if (PFinvalidFd(fd)){
		PFerrno= PFE_FD;
		return(PFerrno);
	}

	if (PFftab[fd].hdr.firstfree != PF_PAGE_LIST_END){
		/* get a page from the free list */
		*pagenum = PFftab[fd].hdr.firstfree;
		if ((error=PFbufGet(fd,*pagenum,&fpage,PFreadfcn,
					PFwritefcn))!= PFE_OK)
			/* can't get the page */
			return(error);
		PFftab[fd].hdr.firstfree = fpage->nextfree;
		PFftab[fd].hdrchanged = TRUE;
	}
	else {
		/* Free list empty, allocate one more page from the file */
		*pagenum = PFftab[fd].hdr.numpages;
		if ((error=PFbufAlloc(fd,*pagenum,&fpage,PFwritefcn))!= PFE_OK)
			/* can't allocate a page */
			return(error);
	
		/* increment # of pages for this file */
		PFftab[fd].hdr.numpages++;
		PFftab[fd].hdrchanged = TRUE;

		/* mark this page dirty */
		if ((error=PFbufUsed(fd,*pagenum))!= PFE_OK){
			printf("internal error: PFalloc()\n");
			exit(1);
		}

	}

	/* zero out the page. Seems to be a nice thing to do,
	at least for debugging. */
	/*
	bzero(fpage->pagebuf,PF_PAGE_SIZE);
	*/

	/* Mark the new page used */
	fpage->nextfree = PF_PAGE_USED;

	/* set return value */
	*pagebuf = fpage->pagebuf;
	
	return(PFE_OK);
}

PF_DisposePage(fd,pagenum)
int fd;		/* file descriptor */
int pagenum;	/* page number */
/****************************************************************************
SPECIFICATIONS:
	Dispose the page numbered "pagenum" of the file "fd".
	Only a page that is not fixed in the buffer can be disposed.

AUTHOR: clc

RETURN VALUE:
	PFE_OK	if no error.
	PF error code if error.

*****************************************************************************/
{
PFfpage *fpage;	/* pointer to file page */
int error;

	if (PFinvalidFd(fd)){
		PFerrno = PFE_FD;
		return(PFerrno);
	}

	if (PFinvalidPagenum(fd,pagenum)){
		PFerrno = PFE_INVALIDPAGE;
		return(PFerrno);
	}

	if ((error=PFbufGet(fd,pagenum,&fpage,PFreadfcn,PFwritefcn))!= PFE_OK)
		/* can't get this page */
		return(error);
	
	if (fpage->nextfree != PF_PAGE_USED){
		/* this page already freed */
		if (PFbufUnfix(fd,pagenum,FALSE)!= PFE_OK){
			printf("internal error: PFdispose()\n");
			exit(1);
		}
		PFerrno = PFE_PAGEFREE;
		return(PFerrno);
	}

	/* put this page into the free list */
	fpage->nextfree = PFftab[fd].hdr.firstfree;
	PFftab[fd].hdr.firstfree = pagenum;
	PFftab[fd].hdrchanged = TRUE;

	/* unfix this page */
	return(PFbufUnfix(fd,pagenum,TRUE));
}

PF_UnfixPage(fd,pagenum,dirty)
int fd;	/* file descriptor */
int pagenum;	/* page number */
int dirty;	/* true if file is dirty */
/****************************************************************************
SPECIFICATIONS:
	Tell the Paged File Interface that the page numbered "pagenum"
	of the file "fd" is no longer needed in the buffer.
	Set the variable "dirty" to TRUE if page has been modified.

AUTHOR: clc

RETURN VALUE:
	PFE_OK	if no error
	PF error code if error.

*****************************************************************************/
{

	if (PFinvalidFd(fd)){
		PFerrno = PFE_FD;
		return(PFerrno);
	}

	if (PFinvalidPagenum(fd,pagenum)){
		PFerrno = PFE_INVALIDPAGE;
		return(PFerrno);
	}

	return(PFbufUnfix(fd,pagenum,dirty));
}

/* error messages */
static char *PFerrormsg[]={
"No error",
"No memory",
"No buffer space",
"Page already fixed in buffer",
"page to be unfixed is not in the buffer",
"unix error",
"incomplete read of page from file",
"incomplete write of page to file",
"incomplete read of header from file",
"incomplete write of header from file",
"invalid page number",
"file already open",
"file table full",
"invalid file descriptor",
"end of file",
"page already free",
"page already unfixed",
"new page to be allocated already in buffer",
"hash table entry not found",
"page already in hash table"
};

void PF_PrintError(s)
char *s;	/* string to write */
/****************************************************************************
SPECIFICATIONS:
	Write the string "s" onto stderr, then write the last
	error message from PF onto stderr.

AUTHOR: clc

RETURN VALUE: none

*****************************************************************************/
{

	fprintf(stderr,"%s",s);
	fprintf(stderr,":%s",PFerrormsg[-1*PFerrno]);
	if (PFerrno == PFE_UNIX)
		/* print the unix error message */
		perror(" ");
	else	fprintf(stderr,"\n");

}


PF_MergeSort(fname)
char* fname;	/* filename for mergesort */
/****************************************************************************
SPECIFICATIONS:
	Takes a file name as input and does merge sort on it.

AUTHOR: Aashish, Anand and Dibyendu

RETURN VALUE: none

*****************************************************************************/
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