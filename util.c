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

  // length we need for new dir/file
  int need_length = 4 * ((8 + n_len + 3) / 4);

  for(i = 0; i < 12; i++) // search direct blocks only
  {
    if(parent->INODE.i_block[i] == 0)
    {
      return -1;
    }

    // parents ith block
    get_block(parent->dev, parent->INODE.i_block[i], buf);

    dp = (DIR *)buf;
    cp = buf;

    // traverse directories until we reach the last dir
    while(cp + dp->rec_len < buf + BLKSIZE)
    {
      cp+= dp->rec_len;
      dp = (DIR *)cp;
    }

    ideal_len = 4 * ((8 + dp->name_len + 3) / 4);

    // get the remaining size of dir block
    remain = dp->rec_len - ideal_len;

    // if we have more remaining size than we need we can add dir
    if(remain >= need_length)
    {
      // set dir rec_len to ideal len
      dp->rec_len = ideal_len;

      // increment dp,cp one more time
      cp += dp->rec_len;
      dp = (DIR *)cp;

      // add new dir/file
      dp->rec_len = remain;
      dp->inode = myino;
      dp->name_len = n_len;
      strncpy(dp->name, myname, n_len);
    }
    else // no more room! we need to create a new block
    {
      // allocate new block
      int bno = balloc(parent->dev);
      // increse parentes inode size by 1024 (blksize)
      parent->INODE.i_size += 1024;

      // get parents 0th block into buf
      get_block(parent->dev, parent->INODE.i_block[0], buf);

      dp = (DIR *)buf;

      // add dir as first item in block
      dp->rec_len = BLKSIZE;
      dp->inode = myino;
      dp->name_len = n_len;
      strncpy(dp->name, myname, n_len);
    }

    // write block back to disk
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

int truncate(MINODE *mip)
{
  for (int i = 0; i < 15; i++)
  {
    if (mip->INODE.i_block[i] == 0)
    {
      continue;
    }
    bdealloc(mip->dev, mip->INODE.i_block[i]);
  }
  if(mip->INODE.i_block[12] != 0) // there are indirect blocks to deallocate
  {
    int ibuf[256];
    get_block(mip->dev, mip->INODE.i_block[12], ibuf);
    for(int i = 0; i < 256; i++) // indirect has 256 blocks, so go through all
    {
      if(ibuf[i] == 0) // empty block
        continue;
      bdealloc(mip->dev, ibuf[i]); // deallocate a block if it is not empty
    }
  }
  if(mip->INODE.i_block[13] != 0) // double indirect blocks
  {
    int ibuf[256];
    int tempBuf[256];
    get_block(mip->dev, mip->INODE.i_block[13], ibuf);
    for(int i = 0; i < 256; i++) // 256 indirect blocks
    {
      if(ibuf[i] != 0)
      {
        get_block(mip->dev, ibuf[i], tempBuf);
        for(int j = 0; j < 256; j++) // each 256 indirect has 256 double indirect blocks
        {
          if(tempBuf[j] == 0)
            continue;
          bdealloc(mip->dev, tempBuf[j]); // deallocate block
        }
        bdealloc(mip->dev, ibuf[i]); // deallocate full block
      }
    }
  }
  // touch mips teime and set it dirty
  mip->INODE.i_atime = time(0L);
  mip->INODE.i_mtime = time(0L);
  mip->INODE.i_ctime = time(0L);
  mip->INODE.i_size = 0;
  mip->dirty = 1;
}

int my_lseek()
{
  int original_position;
  int fd, position;

  // scan in fd and position
  sscanf(pathname, "%d", &fd);
  sscanf(link, "%d", &position);

  if(running->fd[fd] == NULL)
  {
    printf("Error, bad fd!\n");
    return -1;
  }

  OFT *oftp = running->fd[fd];
  // check if position over runs either end of file
  if(oftp->mptr->INODE.i_size < position)
  {
    printf("Error, postion greater than file size!\n");
    return -1;
  }

  // get the position of the opened files offset
  original_position = oftp->offset;
  // set offset to position
  oftp->offset = position;

  return original_position;
}
