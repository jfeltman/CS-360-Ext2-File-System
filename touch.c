
int touch()
{

  MINODE *mip;

  int ino = getino(dev, pathname);

  mip = iget(dev, ino);

  INODE *ip = &mip->INODE;

  ip->i_atime = time(0L);
  ip->i_mtime = time(0L);
  ip->i_ctime = time(0L);

  iput(mip);
}
