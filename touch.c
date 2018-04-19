int touch()
{

  MINODE *mip;

  int ino = getino(dev, pathname);
  if(ino == 0)
  {
    printf("Error, invalid pathname!\n");
    return -1;
  }

  mip = iget(dev, ino);

  INODE *ip = &mip->INODE;

  ip->i_atime = time(0L);
  ip->i_mtime = time(0L);
  ip->i_ctime = time(0L);

  iput(mip);
}

int my_chmod()
{
  MINODE *mip;
  int newMode;

  int ino = getino(dev, pathname);
  if(ino == 0)
  {
    printf("Error, invalid pathname!\n");
    return -1;
  }
  mip = iget(dev, ino);

  // print and scan in an octal notation
  printf("File found! with mode: %o\n", mip->INODE.i_mode);
  sscanf(link, "%o", &newMode);

  // The modes are in octal
  // 0100000 - REG File
  // 0120000 - LNK FILE
  // 0040000 - DIR

  if(S_ISREG(mip->INODE.i_mode))
  {
    mip->INODE.i_mode = 0100000 + newMode;
  }
  else if (S_ISLNK(mip->INODE.i_mode))
  {
    mip->INODE.i_mode = 0120000 + newMode;
  }
  else if (S_ISDIR(mip->INODE.i_mode))
  {
    mip->INODE.i_mode = 0040000 + newMode;
  }

  mip->dirty = 1;

  printf("INODE->mode = %o\n", mip->INODE.i_mode);

  iput(mip);
}

int my_stat()
{
  MINODE *mip;
  char mydate[32], *dateCopy, timeAndDate[32];

  int ino = getino(dev, pathname);
  if(ino == 0)
  {
    printf("Error, invalid pathname!\n");
    return -1;
  }

  mip = iget(dev, ino);

  dateCopy = mydate;
  dateCopy = (char *)ctime(&mip->INODE.i_ctime);
  dateCopy += 4;
  strncpy(timeAndDate, dateCopy, 12);
  timeAndDate[12] = 0;

  printf("***STATS***\n");
  printf("dev = %d\n", mip->dev);
  printf("ino = %d\n", mip->ino);
  printf("mode = %o\n", mip->INODE.i_mode);
  printf("uid = %d\n", mip->INODE.i_uid);
  printf("gid = %d\n", mip->INODE.i_gid);
  printf("nlinks = %d\n", mip->INODE.i_links_count);
  printf("size = %d\n", mip->INODE.i_size);
  printf("time = %s\n", timeAndDate);

  iput(mip);
}
