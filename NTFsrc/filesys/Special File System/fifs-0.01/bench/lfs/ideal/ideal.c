#include <stdio.h>
//#include <sys/time.h>
//#include <sys/resource.h>
#include <fcntl.h>
#include <stdlib.h>
#include "unix.h"

int BRANCH_OUT;    // 25
int FILES_PER_DIR; // 25

char *buf;
int bufsize;
int filesize;
int num_levels;
int num_files;  
int files;
int total_data;
char root_dir[128];

void init()
{
  char s[128];
 
  sprintf(s,"%s/tree",root_dir); 
  
  if (access(s,0777) == -1)
     mkdir(s,0777);
  chdir(s);
}

void create_files(levels)
int levels;
{
  int i,j;
  int fd;
  char filename[128], dirname[128];


  if (files==0) return;
      for (i=0; (i<files)&&(i<FILES_PER_DIR); i++)
      {
          sprintf(filename,"file%d",i);
          if ((fd = open(filename,O_RDWR | O_CREAT, 0666))==-1)
             printf("Error : unable to open file\n");
          for (j=filesize; j>bufsize; j-=bufsize)
              write(fd,buf,bufsize);
	  if (j>0) 
             write(fd,buf,j);
          close(fd);
      }

  files -= i;

  if (levels>1)
      for (i=0; i<BRANCH_OUT; i++)
      {
         sprintf(dirname,"dir%d",i);
	 mkdir(dirname, 0777);
         chdir(dirname);
         create_files(levels-1, files);
         chdir("..");
	 if (files == 0)
	   break;
      }     
  return;
}

void read_files(levels)
int levels;
{
  int i,j;
  int fd;
  char filename[128], dirname[128];

  if (files==0) return;
      for (i=0; (i<files)&&(i<FILES_PER_DIR); i++)
      {
          sprintf(filename,"file%d",i);
          if ((fd = open(filename,O_RDONLY,0666)) == -1)
             printf("Error : unable to open file\n");
          for (j=filesize; j>bufsize; j-=bufsize)
              read(fd,buf,bufsize);
	  if (j>0) 
             read(fd,buf,j);
          close(fd);
      }

  files -= i;

  if (levels>1)
      for (i=0; i<BRANCH_OUT; i++)
      {
         sprintf(dirname,"dir%d",i);
         chdir(dirname);
         read_files(levels-1, files);
         chdir("..");
	 if (files == 0)
	   break;
      }     
  return;
}

void write_files(levels)
int levels;
{
  int i,j;
  int fd;
  char filename[128], dirname[128];


  if (files==0) return;
      for (i=0; (i<files)&&(i<FILES_PER_DIR); i++)
      {
          sprintf(filename,"file%d",i);
          if ((fd = open(filename,O_WRONLY, 0666))==-1)
             printf("Error : unable to open file\n");
          for (j=filesize; j>bufsize; j-=bufsize)
              write(fd,buf,bufsize);
	  if (j>0) 
             write(fd,buf,j);
          close(fd);
      }

  files -= i;

  if (levels>1)
      for (i=0; i<BRANCH_OUT; i++)
      {
         sprintf(dirname,"dir%d",i);
         chdir(dirname);
         write_files(levels-1, files);
         chdir("..");
	 if (files == 0)
	   break;
      }     
  return;
}

void write_files_sync(levels)
int levels;
{
  int i,j;
  int fd;
  char filename[128], dirname[128];


  if (files==0) return;
      for (i=0; (i<files)&&(i<FILES_PER_DIR); i++)
      {
          sprintf(filename,"file%d",i);
          if ((fd = open(filename,O_WRONLY, 0666))==-1)
             printf("Error : unable to open file\n");
          for (j=filesize; j>bufsize; j-=bufsize)
              write(fd,buf,bufsize);
	  if (j>0) 
             write(fd,buf,j);
	  fsync(fd);
	  close(fd);
      }

  files -= i;

  if (levels>1)
      for (i=0; i<BRANCH_OUT; i++)
      {
         sprintf(dirname,"dir%d",i);
         chdir(dirname);
         write_files(levels-1, files);
         chdir("..");
	 if (files == 0)
	   break;
      }     
  return;
}

void delete_files(levels)
int levels;
{
  int i;
  int fd;
  char filename[128], dirname[128];

      if (files==0) return;
      for (i=0; (i<files)&&(i<FILES_PER_DIR); i++)
      {
          sprintf(filename,"file%d",i);
          unlink(filename);
      }

      files -= i;
      if (levels>1)
      for (i=0; i<BRANCH_OUT; i++)
      {
         sprintf(dirname,"dir%d",i);
         chdir(dirname);
         delete_files(levels-1);
         chdir("..");
         rmdir(dirname);
	 if (files == 0)
	   break;
      }     
  return;
}


void rw(choice)
     int choice;

{
  double elapsed_micros, rate, micros;
  struct timeval start, end;
//  struct rusage begin, finish;
  int numfiles;
  
//  getrusage(RUSAGE_SELF, &begin);
  gettimeofday(&start, (struct timezone *) NULL);

  switch(choice)
    {
      case 1 :  create_files(num_levels);
                break;

      case 2 :  delete_files(num_levels);
                break;

      case 3 :  read_files(num_levels);
                break;

      case 4 :  write_files(num_levels);
	        break;
   
      case 5 :  write_files_sync(num_levels);
	        break;
      }

//   if (choice!= 3)
//     sync();

  gettimeofday(&end, (struct timezone *) NULL);
//  getrusage(RUSAGE_SELF, &finish);
  
//   micros = (finish.ru_utime.tv_sec + finish.ru_stime.tv_sec
// 	    - begin.ru_utime.tv_sec - begin.ru_stime.tv_sec)*1000000
// 	      + (finish.ru_utime.tv_usec - begin.ru_utime.tv_usec) +
// 		(finish.ru_stime.tv_usec - begin.ru_stime.tv_usec);
  
  elapsed_micros = (end.tv_sec - start.tv_sec)*1000000 +
    (end.tv_usec - start.tv_usec);
  
  chdir("..");

  numfiles = total_data/filesize;
  rate = (double) (numfiles*filesize)/elapsed_micros;

  printf ( "%2.4f\n", rate );

  return;
}

int
main(argc,argv)
int argc;
char *argv[];
{
   int choice, temp;

   unix_init();
   if (argc!=7)

   {
      printf("Usage: rw root_dir filesize total_data branching filer_per_dir option\n"); 
      printf("Options:\n");
      printf("\t1:  create\n");
      printf("\t2:  delete\n");
      printf("\t3:  read\n");
      printf("\t4:  write\n");
      printf("\t5:  write w/ sync\n");
      printf("Example: rw dir 1 1000 100 100 1\n");
      printf("\tcreates 1000 1KB files in sets of 100/dir, creating 100 dirs\n");
      return;
   }

   strcpy(root_dir, argv[1]);
   filesize = atoi(argv[2])*1024;
   total_data = atoi(argv[3])*1024;
   BRANCH_OUT = atoi(argv[4]);
   FILES_PER_DIR = atoi(argv[5]);


   num_files = total_data/filesize;
   temp = num_files-1;
   for (num_levels=0; temp>0; num_levels++)
       temp /= BRANCH_OUT;
   if (filesize<4*1024*1024)
      bufsize = filesize;
   else 
      bufsize = 4*1024*1024;

   buf = (char *) malloc(bufsize);
  
   init();
   files = num_files;
   rw(atoi(argv[6]));
   return 0;
}
            
            
