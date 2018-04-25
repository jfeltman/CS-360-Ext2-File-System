/************* cd_ls_pwd.c file **************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

#include "util.h"

extern MINODE minode[NMINODE];
extern MINODE *root;

extern PROC   proc[NPROC], *running;
extern MNTABLE mntable, *mntPtr;

extern SUPER *sp;
extern GD    *gp;
extern INODE *ip;

extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, iblk;
extern char line[128], cmd[32], pathname[64];

extern char gpath[128];   // hold tokenized strings
extern char *name[64];    // token string pointers
extern int  n;            // number of token strings

extern MINODE minode[ ];
extern PROC *running;
extern int fd, dev;
extern int bmap, imap, iblk;


extern char gpath[128];   // hold tokenized strings
extern char *name[64];    // token string pointers
extern int  n;            // number of token strings

extern MINODE *iget();


#define OWNER  000700
#define GROUP  000070
#define OTHER  000007

change_dir()
{
  char temp[256];
  char buf[BLKSIZE];
  DIR *dp;
  MINODE *ip, *newip, *cwd;
  int dev, ino;
  char c;

  // if not path set running cwd to root MINODE
  if (pathname[0] == 0){
     iput(running->cwd);
     running->cwd = iget(root->dev, 2);
     return;
  }

  if (pathname[0] == '/')
  {
    dev = root->dev;
  }
  else
  {
    dev = running->cwd->dev;
  }

  strcpy(temp, pathname);
  ino = getino(dev, temp);

  if (!ino){
     printf("cd : no such directory\n");
     return(-1);
  }
  printf("dev=%d ino=%d\n", dev, ino);
  newip = iget(dev, ino);    /* get inode of this ino */

  printf("mode=%4x   ", newip->INODE.i_mode);

  if (!S_ISDIR(newip->INODE.i_mode)){
     printf("%s is not a directory\n", pathname);
     iput(newip);
     return(-1);
  }

  iput(running->cwd);
  running->cwd = newip;

  printf("after cd : cwd = [%d %d]\n", running->cwd->dev, running->cwd->ino);
}

int ls_file(MINODE *mip, char *name)
{
  int k;
  u16 mode, mask;
  char mydate[32], *s, *cp, ss[32];

  mode = mip->INODE.i_mode;
  if (S_ISDIR(mode))
      putchar('d');
  else if (S_ISLNK(mode))
      putchar('l');
  else // reg file
      putchar('-');

   mask = 000400; // bitwise operation to print out permssions
   for (k=0; k<3; k++){
      if (mode & mask)
         putchar('r');
      else
         putchar('-');
      mask = mask >> 1;

     if (mode & mask)
        putchar('w');
     else
        putchar('-');
        mask = mask >> 1;

     if (mode & mask)
        putchar('x');
     else
        putchar('-');
        mask = mask >> 1;

     }
     printf("%4d", mip->INODE.i_links_count);
     printf("%4d", mip->INODE.i_uid);
     printf("%4d", mip->INODE.i_gid);
     printf("  ");

     s = mydate;
     s = (char *)ctime(&mip->INODE.i_ctime);
     s = s + 4;
     strncpy(ss, s, 12);
     ss[12] = 0;

     printf("%s", ss);
     printf("%8ld ",   mip->INODE.i_size);

     printf("%4s", name);

     if (S_ISLNK(mode)) // if symlink print file its linked to
        printf(" -> %s", (char *)mip->INODE.i_block);
     printf("\n");
}

int ls_dir(MINODE *mip)
{
  int i;
  char sbuf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;
  MINODE *dip;

  for (i=0; i<12; i++){ /* search direct blocks only */
     printf("i_block[%d] = %d\n", i, mip->INODE.i_block[i]);
     if (mip->INODE.i_block[i] == 0)
         return 0;

     get_block(mip->dev, mip->INODE.i_block[i], sbuf);
     dp = (DIR *)sbuf;
     cp = sbuf;

     // step through each dir
     while (cp < sbuf + BLKSIZE){
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = 0;

        // get each files MINODE
        dip = iget(dev, dp->inode);
        ls_file(dip, temp); // call ls_file on it
        iput(dip); // dispose of MINODE

        cp += dp->rec_len;
        dp = (DIR *)cp;
     }
  }
}

int list_file()
{
  MINODE *mip;
  u16 mode;
  int dev, ino;

  // no path so just ls the root dir
  if (pathname[0] == 0)
    ls_dir(running->cwd);
  else{
    dev = root->dev;
    ino = getino(dev, pathname); // get ino of pathname
    if (ino==0){
      printf("no such file %s\n", pathname);
      return -1;
    }
    // get pathnames MINODE and mode
    mip = iget(dev, ino);
    mode = mip->INODE.i_mode;
    if (!S_ISDIR(mode)) // file
      ls_file(mip, (char *)basename(pathname));
    else
      ls_dir(mip); // dir
    iput(mip);
  }
}

int rpwd(MINODE *wd)
{
  char buf[BLKSIZE], myname[256], *cp;
  MINODE *parent, *ip;
  u32 myino, parentino;
  DIR   *dp;

  if (wd == root)
      return;

  parentino = findino(wd, &myino); // find parent ino
  parent = iget(dev, parentino); // get parent MINODE

  findmyname(parent, myino, myname); // get the name
  // recursively call rpwd()
  rpwd(parent);

  iput(parent);
  printf("/%s", myname); //print out name found

  return 1;
}

char *pwd(MINODE *wd)
{
  if (wd == root){
    printf("/");
    return;
  }
  rpwd(wd);
  printf("\n");
}
