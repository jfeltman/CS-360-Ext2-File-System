int read_file()
{
  char filename[128], temp[128];
  char buf[BLKSIZE];
  int nbytes, count;

  strcpy(temp, pathname);
  strcpy(filename, temp); //puts the file name in filename

  sscanf(link, "%d", &nbytes); // takes the 2nd parameter and makes it an int

  printf("Pass\n");
  if (running->fd[fd] == NULL || (running->fd[fd]->mode != 0 && running->fd[fd]->mode != 2))
  {
    printf("File not opened for Read\n");
    return -1;
  }
  count = myread(fd, buf, nbytes); // call read to get count as well as set the buf

  printf("************ %s ************\n", filename);
  printf("%s\n", buf);
  printf("*********************************\n");
  printf("myread: read %d char from file descriptor %d\n", count, fd);
  memset(buf, 0, BLKSIZE); // reset buf
  return count;
}

int myread(int fd, char *buf, int nbytes)
{
  MINODE *mip;
  OFT *oftp;

  int count = 0;
  int lbk, startbyte, blk, remain, dblk, copyBits;
  int ibuf[256], dbuf[256];
  char readbuf[BLKSIZE];

  oftp = running->fd[fd]; //sets the oftp to the running opened file
  mip = oftp->mptr; //sets a mip to ofts MINODE

  int avail = mip->INODE.i_size - oftp->offset; //mip->INODE.i_size = filesize

  char *cq = buf;

  while (nbytes && avail)
  {
    lbk = oftp->offset / BLKSIZE;
    startbyte = oftp->offset % BLKSIZE;

    if (lbk < 12 ) //direct blocks
    {
      blk = mip->INODE.i_block[lbk]; //directly set blk to the correct block #
    }
    else if (lbk >= 12 && lbk < 256 + 12)
    {
      //indirect blocks
    }
    else
    {
      //double indirect
    }

    get_block(mip->dev, blk, readbuf); //get the information from the file into readbuf

    char *cp = readbuf + startbyte;
    remain = BLKSIZE - startbyte;

    if (avail < nbytes)
      copyBits = avail;
    else
      copyBits = nbytes;

    strncpy(buf, cp, copyBits);

    count += copyBits;
    remain -= copyBits;
    nbytes -= copyBits;
    avail -= copyBits;
    oftp->offset += copyBits;

    buf[count] = NULL;

    if (nbytes <= 0 || avail <= 0) //check to see if we've reached the end
    {
      break;
    }
  }

  return count;
}
