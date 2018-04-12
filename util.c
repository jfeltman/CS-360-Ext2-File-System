/*********** util.c file ****************/

#include "util.h"

int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   read(dev, buf, BLKSIZE);
}
int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   write(dev, buf, BLKSIZE);
}

int findmyname(MINODE *parent, u32 myino, char *myname)
{
 int i;
 char buf[BLKSIZE], temp[256], *cp;
 DIR    *dp;
 MINODE *mip = parent;

 //**********  search for a file name ***************
 for (i=0; i<12; i++){ // search direct blocks only
     if (mip->INODE.i_block[i] == 0)
           return -1;

     get_block(mip->dev, mip->INODE.i_block[i], buf);
     dp = (DIR *)buf;
     cp = buf;

     while (cp < buf + BLKSIZE){
       strncpy(temp, dp->name, dp->name_len);
       temp[dp->name_len] = 0;
       //printf("%s  ", temp);

       if (dp->inode == myino){
           strncpy(myname, dp->name, dp->name_len);
           myname[dp->name_len] = 0;
           return 0;
       }
       cp += dp->rec_len;
       dp = (DIR *)cp;
     }
 }
 return -1;
}

int findino(MINODE *mip, u32 *myino)
{
  char buf[BLKSIZE], *cp;
  DIR *dp;

  get_block(mip->dev, mip->INODE.i_block[0], buf);
  cp = buf;
  dp = (DIR *)buf;
  *myino = dp->inode;
  cp += dp->rec_len;
  dp = (DIR *)cp;
  return dp->inode;
}

int tst_bit(char *buf, int bit)
{
  int i, j;
  i = bit / 8;  j = bit % 8;
  if (buf[i] & (1 << j))
     return 1;
  return 0;
}

int set_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] |= (1 << j);
}

int clr_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] &= ~(1 << j);
}

int decFreeInodes(int dev)
{
  char buf[BLKSIZE];

  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count--;
  put_block(dev, 2, buf);
}

int decFreeBlocks(int dev)
{
  char buf[BLKSIZE];

  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_blocks_count--;
  put_block(dev, 2, buf);
}

int incFreeInodes(int dev)
{
  char buf[BLKSIZE];

  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count++;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count++;
  put_block(dev, 2, buf);
}

int incFreeBlocks(int dev)
{
  char buf[BLKSIZE];

  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count++;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_blocks_count++;
  put_block(dev, 2, buf);
}

int ialloc(int dev)
{
  int  i;
  char buf[BLKSIZE];
  // read inode_bitmap block
  get_block(dev, imap, buf);

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       decFreeInodes(dev);

       put_block(dev, imap, buf);

       return i+1;
    }
  }
  printf("ialloc(): no more free inodes\n");
  return 0;
}

int balloc(int dev)
{
    int i;
    char buf[BLKSIZE];
    // read block bitmap into buf
    get_block(dev, bmap, buf);

//    printf("**** BALLOC ****\n");
    for(i = 0; i < nblocks; i++)
    {
        // allocate a free block
        if(tst_bit(buf, i)==0)
        {
            set_bit(buf, i);
            decFreeBlocks(dev);
            put_block(dev, bmap, buf);
            // return i which is its block number
            return i;
        }
    }
    return 0;
}

int enter_name(MINODE *pip, int myino, char *myname)
{
  int i, ideal_len, remain;
  char buf[BLKSIZE];
  DIR *dp;
  char *cp;

  MINODE *parent = pip;
  int n_len = strlen(myname);

  int need_length = 4 * ((8 + n_len + 3) / 4);

  for(i = 0; i < 12; i++) // search direct blocks only
  {
    if(parent->INODE.i_block[i] == 0)
    {
      return -1;
    }

    get_block(parent->dev, parent->INODE.i_block[i], buf);

    dp = (DIR *)buf;
    cp = buf;

    while(cp + dp->rec_len < buf + BLKSIZE)
    {
      cp+= dp->rec_len;
      dp = (DIR *)cp;
    }

    ideal_len = 4 * ((8 + dp->name_len + 3) / 4);

    remain = dp->rec_len - ideal_len;

    if(remain >= need_length)
    {
      dp->rec_len = ideal_len;

      cp += dp->rec_len;
      dp = (DIR *)cp;

      dp->rec_len = remain;
      dp->inode = myino;
      dp->name_len = n_len;
      strncpy(dp->name, myname, n_len);
    }
    else
    {
      int bno = balloc(parent->dev);
      parent->INODE.i_size += 1024;

      get_block(parent->dev, parent->INODE.i_block[0], buf);

      dp = (DIR *)buf;

      dp->rec_len = BLKSIZE;
      dp->inode = myino;
      dp->name_len = n_len;
      strncpy(dp->name, myname, n_len);
    }

    put_block(parent->dev, parent->INODE.i_block[i], buf);
  }
}

int bdealloc(int dev, int block_num)
{
  char buf[BLKSIZE];

  get_block(dev, bmap, buf);
  clr_bit(buf, block_num - 1);
  put_block(dev, bmap, buf);
  incFreeBlocks(dev);
}
int idealloc(int dev, int myino)
{
  char buf[BLKSIZE];

  get_block(dev, imap, buf);
  clr_bit(buf, myino - 1);
  put_block(dev, imap, buf);
  incFreeInodes(dev);
}
