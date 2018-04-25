int write_file()
{
  char buf[BLKSIZE];
  int nbytes;

  if(running->fd[fd] == NULL || running->fd[fd]->mode == 0)
  {
    printf("Error, file not opened for write!\n");
    return -1;
  }

  printf("enter a string: ");
  fgets(buf, BLKSIZE, stdin);

  nbytes = strlen(buf) - 1;
  buf[nbytes] = NULL;

  return(myWrite(fd, buf, nbytes)); // returns count of bits written
}

int myWrite(int fd, char *buf, int nbytes)
{
  MINODE *mip;
  OFT *oftp;

  char wbuf[BLKSIZE];
  int lbk, blk, startByte, ibuf[256], remain, copyBits;
  int count = 0;

  oftp = running->fd[fd]; // get running opened file
  mip = oftp->mptr; // get opened files MINODE

  while(nbytes > 0)
  {
    lbk = oftp->offset / BLKSIZE; // logical block size
    startByte = oftp->offset % BLKSIZE; // startbyte is for appending to a file

    if(lbk < 12) // direct blocks
    {
      if(mip->INODE.i_block[lbk] == 0)
      {
        mip->INODE.i_block[lbk] = balloc(mip->dev); // if no block allocate a new one
      }
      blk = mip->INODE.i_block[lbk]; // set blk to new logical block
    }
    else if(lbk >= 12 && lbk < 256 + 12) // indriect blocks
    {
      if(mip->INODE.i_block[12] == 0)
      {
        mip->INODE.i_block[12] = balloc(mip->dev);
        // zero out the block on disk?
      }
      get_block(fd, mip->INODE.i_block[12], ibuf);
      blk = ibuf[lbk - 12];

      if(blk == 0)
      {
        blk = balloc(dev);
        mip->INODE.i_block[12] = blk;
      }
    }
    else // double indirect blocks
    {
      int dbuf[256], dblk;
      if(mip->INODE.i_block[13] == 0)
      {
        mip->INODE.i_block[13] = balloc(mip->dev);
      }

      get_block(fd, mip->INODE.i_block[13], ibuf);
      lbk -= (12 + 256);
      dblk = ibuf[lbk/256];

      if(dblk == 0)
      {
        dblk = balloc(dev);
      }
      get_block(fd, dblk, dbuf);
      blk = dbuf[lbk % 256];

      if(blk == 0)
      {
        blk = balloc(dev);
      }
    }

    get_block(mip->dev, blk, wbuf);   // read disk block into wbuf[ ]
    char *cp = wbuf + startByte;      // cp points at startByte in wbuf[]
    remain = BLKSIZE - startByte;     // number of BYTEs remain in this block

    if(remain < nbytes)
      copyBits = remain;
    else
      copyBits = nbytes;

    // copy into cp (which is wbuf) the string we wrote
    strncpy(cp, buf, copyBits);
    count += copyBits;
    remain -= copyBits;
    oftp->offset += copyBits;
    if (oftp->offset > mip->INODE.i_size)  // especially for RW|APPEND mode
    {
      mip->INODE.i_size += copyBits;    // inc file size (if offset > fileSize)
    }
    nbytes -= copyBits; // decrement nbytes by copybit

    put_block(mip->dev, blk, wbuf);   // write wbuf[ ] to disk
  }

  mip->dirty = 1;
  printf("wrote %d chars into fd = %d\n", count, fd);
  return count;
}
