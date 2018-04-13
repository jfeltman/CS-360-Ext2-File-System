int rm_child(MINODE *parent, char *name)
{
  MINODE *tempParent = parent;

  int i, cur_rec_len, prev_rec_len;
  char buf[BLKSIZE];
  char *cp, tempName[128];
  DIR *dp;

  for(i = 0; i < 12; i++)
  {
    if(tempParent->INODE.i_block[i] == 0)
      return -1;

    get_block(tempParent->dev, tempParent->INODE.i_block[i], buf);

    dp = (DIR *)buf;
    cp = buf;

    while(cp < buf + BLKSIZE)
    {
      strncpy(tempName, dp->name, dp->name_len);
      tempName[dp->name_len] = NULL;

      if(strcmp(tempName, name) == 0)
      {
        cur_rec_len = dp->rec_len;

        // checking if last dir in block
        if(cp + cur_rec_len >= buf + BLKSIZE) // goes over block
        {
          cp -= prev_rec_len;
          dp = (DIR *)cp;
          dp->rec_len += cur_rec_len;

          put_block(tempParent->dev, tempParent->INODE.i_block[i], buf);
          tempParent->dirty = 1;
          return 1;
        }
      }

      prev_rec_len = dp->rec_len;
      cp += dp->rec_len;
      dp = (DIR *)cp;
      memset(tempName, 0, 128);
    }
    printf("no Directory!!!\n");
    return -1;
  }
}

int remove_dir()
{
  MINODE *mip, *pip;
  int pino;
  char parent[64], child[32], temp[64];
  strcpy(temp, pathname);
  strcpy(parent, dirname(temp));
  strcpy(temp, pathname);
  strcpy(child, basename(temp));

  int ino = getino(&dev, pathname);
  mip = iget(dev, ino);

  if(!S_ISDIR(mip->INODE.i_mode)) //check to see if it actually is a dir
  {
    printf("Invalid pathname, not a dir!\n");
    return -1;
  }
  if(mip->INODE.i_links_count == 2)
  {
    if (mip->INODE.i_links_count > 2) //noting in dir
    {
      printf("DIR NOT EMPTY!\n");
      return -1;
    }
  }
  for (int i = 0; i < 12; i++)
  {
    if (mip->INODE.i_block[i] == 0)
    {
      continue;
    }
    bdealloc(dev, mip->INODE.i_block[i]);
  }
  idealloc(mip->dev, mip->ino);
  iput(mip);

  if(strcmp(parent, ".") == 0)
  {
    strcpy(parent, "");
  }
  if(parent[0] == '/')
  {
    printf("ROOT START\n");
    mip = root;
    dev = root->dev;
  }
  else
  {
    printf("CWD START\n");
    mip = running->cwd;
    dev = running->cwd->dev;
  }

  if(strcmp(parent, "") == 0)
  {
    pino = running->cwd->ino;
  }
  else
  {
    pino = getino(dev, parent);
  }
  printf("pino = %d\n", pino);
  pip = iget(mip->dev, pino); //gets the parent node

  rm_child(pip, child);

  pip->INODE.i_links_count--;
  pip->dirty = 1;
  iput(pip);
  return 1;
}
