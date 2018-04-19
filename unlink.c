int myUnlink()
{
  MINODE *mip, *pip;

  char parent[64], child[32], temp[64];

  if(strcmp(pathname, "") == 0)
  {
    printf("Error no pathname given!\n");
    return -1;
  }

  // get minode of pathname
  int ino = getino(dev, pathname);
  if(ino == 0)
  {
    printf("wrong pathname!\n");
    return -1;
  }
  mip = iget(dev, ino);

  // check if mip is a reg/link file and not a dir
  if(S_ISDIR(mip->INODE.i_mode))
  {
    printf("Error, this is a directory, not a file!n");
    iput(mip);
    return -1;
  }

  // decrement link count by 1
  mip->INODE.i_links_count--;

  // if there are no more links counts, we got to dealloc the block and inode
  if(mip->INODE.i_links_count == 0)
  {
    printf("Inodes link count is 0, deallocating blocks and inode!\n");
    truncate(mip);
  }
  iput(mip); // write mip back to disk

  strcpy(temp, pathname);
  strcpy(parent, dirname(temp));
  strcpy(temp, pathname);
  strcpy(child, basename(temp));

  // get parent MINODE
  int pino = getino(dev, parent);
  pip = iget(dev, pino);

  printf("pino = %d\n", pino);

  // call rm_child on parent MINODE and child name
  rm_child(pip, child);

  iput(pip); // write puip back to disk
}
