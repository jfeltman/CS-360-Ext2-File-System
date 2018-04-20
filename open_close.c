int open_file()
{
  int mode, ino, i;
  MINODE *mip;
  OFT *oftp;

  // take in the mode of the 2nd parameter - 0, 1, 2, 3 only
  sscanf(link, "%d", &mode);
  printf("MODE = %d\n", mode);

  // set the dev
  if(pathname[0] == '/')
  {
    dev = root->dev;
  }
  else
  {
    dev = running->cwd->dev;
  }

  // get the ino of the file
  ino = getino(dev, pathname);
  if(ino == 0)
  {
    printf("Error, invalid pathname!\n");
    return -1;
  }
  //get ino's MINODE ptr
  mip = iget(dev, ino);

  if(!S_ISREG(mip->INODE.i_mode))
  {
    printf("Error, not a REG file!\n");
    iput(mip);
    return -1;
  }

  // check the file permissions
  // if file is already opened with W | RW | APPEND then return error
  for(int j = 0; j < NFD; j++)
  {
    // check if theres open files that have the same MINODE mptr
    if(running->fd[j] != NULL && running->fd[j]->mptr == mip)
    {
      // check if the file opened's mode is not Read or mode we want to open is not read
      if(running->fd[j]->mode > 0 || mode > 0)
      {
        printf("Error, file already opened for WRITING/APPENDING!\n");
        iput(mip);
        return -1;
      }
    }
  }
  // allocate a new OpenFileTable
  oftp = malloc(sizeof(OFT));

  // set oftp's member
  oftp->mode = mode;
  oftp->refCount = 1;
  oftp->mptr = mip;

  switch(mode)
  {
    case 0: // READ
      oftp->offset = 0;
      break;
    case 1: // WRITE
      truncate(mip); // deallocate all of mips blocks
      oftp->offset = 0;
      break;
    case 2: // READ+WRITE
      oftp->offset = 0;
      break;
    case 3: // APPEND
      oftp->offset = mip->INODE.i_size;
      break;
    default:
      printf("Invalid mode!\n");
      iput(mip);
      return(-1);
  }

  // find the first i that has running->fd[i] == null
  for(i = 0; i < NFD; i++)
  {
    if(running->fd[i] == NULL)
    {
      break;
    }
  }

  // set running->fd[i] equal to oftp
  running->fd[i] = oftp;

  // if mode is READ
  if(oftp->mode == 0)
  {
    mip->INODE.i_atime = time(0L);
  }
  else if(oftp->mode > 0) // mode is Write, RW, or Append
  {
    mip->INODE.i_atime = time(0L);
    mip->INODE.i_mtime = time(0L);
  }
  mip->dirty = 1; // mark mip dirty
  iput(mip); // dispose of mip

  return i; // i is the file descriptor
}

int pfd()
{
  printf("-----OPENED FILES-----\n");
  printf("fd  mode    offset  INODE\n");
  // go through all fd's to see if there are any opened files
  for(int i = 0; i < NFD; i++)
  {
    char strMode[8];
    if(running->fd[i] != NULL) // file is open
    {
      if(running->fd[i]->mode == 0)
        strcpy(strMode, "READ");
      else if(running->fd[i]->mode == 1)
        strcpy(strMode, "WRITE");
      else if(running->fd[i]->mode == 2)
        strcpy(strMode, "R/W");
      else if(running->fd[i]->mode == 3)
        strcpy(strMode, "APPEND");

      printf("%d   %s    %d      [%d, %d]\n", i, strMode, running->fd[i]->offset,
        running->fd[i]->mptr->dev, running->fd[i]->mptr->ino);
    }
  }
}

int close_file()
{
  int fd;
  MINODE *mip;
  OFT *oftp;

  sscanf(pathname, "%d", &fd);
  if(fd < 0 || fd > NFD)
  {
    printf("Error! fd out of range!\n");
    return -1;
  }

  if(running->fd[fd] == NULL)
  {
    printf("Error! No file opened at fd %d!\n", fd);
    return -1;
  }

  // set oftp to runnings fd
  oftp = running->fd[fd];
  // remove the file from the running fd list
  running->fd[fd] = NULL;
  oftp->refCount--;

  if(oftp->refCount > 0)
    return 0;
  // if we didnt return, that means thers only one more OFT entry
  // get MINODE of oft
  mip = oftp->mptr;
  // dispoe of MINODE
  iput(mip);
}
