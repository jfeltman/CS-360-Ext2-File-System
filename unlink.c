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
      for(int i = 0; i < 256; i++)
      {
        if(ibuf[i] == 0)
          continue;
        bdealloc(mip->dev, ibuf[i]);
      }
    }
    if(mip->INODE.i_block[13] != 0) // double indirect blocks
    {
      int ibuf[256];
      int tempBuf[256];
      get_block(mip->dev, mip->INODE.i_block[13], ibuf);
      for(int i = 0; i < 256; i++)
      {
        if(ibuf[i] != 0)
        {
          get_block(mip->dev, ibuf[i], tempBuf);
          for(int j = 0; j < 256; j++)
          {
            if(tempBuf[j] == 0)
              continue;
            bdealloc(mip->dev, tempBuf[j]);
          }
          bdealloc(mip->dev, ibuf[i]);
        }
      }
    }
    // dealloc the inode
    idealloc(mip->dev, mip->ino);
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
