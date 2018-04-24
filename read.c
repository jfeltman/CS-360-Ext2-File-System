int read_file()
{
  char filename[128], temp[128];
  char buf[BLKSIZE];
  int nbytes, count;

  strcpy(temp, pathname);
  strcpy(filename, temp); //puts the file name in filename

  sscanf(link, "%d", &nbytes);
  //printf("%d\n", running->fd[fd]);
  printf("Pass\n");
  if (running->fd[fd] == NULL || (running->fd[fd]->mode != 0 && running->fd[fd]->mode != 2))
  {
    printf("File not opened for Read\n");
    return -1;
  }

  printf("%d\n", nbytes);
  count = myread(fd, buf, nbytes);

  printf("************ %s ************\n", filename);
  printf("%s", buf);
  printf("\n");
  printf("*********************************\n");
  printf("myread: read %d char from file descriptor %d\n", count, fd);

  return count;
}

int myread(int fd, char *buf, int nbytes)
{
  MINODE *mip;
  OFT *oftp;

  int count = 0;
  int lbk, startbyte, blk, remain, dblk;
  int ibuf[256], dbuf[256];
  char readbuf[BLKSIZE];

  oftp = running->fd[fd]; //sets the oft
  mip = oftp->mptr; //sets a mip

  int avail = mip->INODE.i_size - oftp->offset; //mip->INODE.i_size = filesize
  //printf("avail = %d\n", avail);

  char *cq = buf;

  while (nbytes && avail)
  {
    lbk = oftp->offset / BLKSIZE;
    //printf("lbk = %d\n", lbk);
    startbyte = oftp->offset % BLKSIZE;

    if (lbk < 12 ) //direct blocks
    {
      blk = mip->INODE.i_block[lbk]; //directly set blk to the correct block #
    }
    else if (lbk >= 12 && lbk < 256 + 12)
    {
      //indirect blocks
      get_block(fd, mip->INODE.i_block[12], ibuf); //get the indirect block
      blk = ibuf[lbk - 12]; //get the correct block num from the indirect block
    }
    else
    {
      //double indirect
      get_block(fd, mip->INODE.i_block[13], dbuf); //get the D-indirect block
      lbk -= (12+256);
      dblk = dbuf[lbk / 256];
      get_block(fd, dblk, dbuf);
      blk = dbuf[lbk % 256];
    }

    get_block(mip->dev, blk, readbuf); //get the information from the file into readbuf

    char *cp = readbuf + startbyte;
    remain = BLKSIZE - startbyte;

    while (remain > 0)
    {
      *cq++ = *cp++;
      oftp->offset++; //increment the offset
      count++; //increment the count
      avail--; nbytes--; remain--; //decrement avail, bytes, and remain
      if (nbytes <= 0 || avail <= 0) //check to see if we've reached the end
      {
        break;
      }

    }
    buf[count] = NULL;
  }

  return count;
}
