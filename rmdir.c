int rm_child(MINODE *parent, char *name)
{
  MINODE *tempParent = parent;

  int i, cur_rec_len, prev_rec_len;
  char buf[BLKSIZE];
  char *cp, tempName[128], *tempCP;
  DIR *dp;

  // loop through direct blocks
  for(i = 0; i < 12; i++)
  {
    if(tempParent->INODE.i_block[i] == 0)
      return -1;

    get_block(tempParent->dev, tempParent->INODE.i_block[i], buf);

    dp = (DIR *)buf;
    cp = buf;
    tempCP = buf;

    // loop through the directories of the current block
    while(cp < buf + BLKSIZE)
    {
      strncpy(tempName, dp->name, dp->name_len);
      tempName[dp->name_len] = NULL;

      if(strcmp(tempName, name) == 0)
      {
        cur_rec_len = dp->rec_len;

        printf("cp + cur_rec_len = %d\n", cp+cur_rec_len);
        printf("buf + BLKSIZE = %d\n", buf + BLKSIZE);

        // checking if dp is the last dir in the block
        if(cp + cur_rec_len >= buf + BLKSIZE)
        {
          // if it is, have cp -= the last dirs rec n_len
          cp -= prev_rec_len;
          // now cast dp to the new cp, so dp->2nd to last dir
          dp = (DIR *)cp;
          // add last dirs rec len to 2nd to last dirs
          dp->rec_len += cur_rec_len;

          // write parent block to disk and mark parent dirty
          put_block(tempParent->dev, tempParent->INODE.i_block[i], buf);
          tempParent->dirty = 1;
          return 1;
        }

        // dir is middle of block
        // keep track of the dir we deleting's rec len
        int original_len = dp->rec_len;

        // loop while current cp is less than the end of the block
        while(cp < buf + BLKSIZE)
        {
          // increment directory
          cp += dp->rec_len;
          dp = (DIR *)cp;
          // update cur len to new dir len
          cur_rec_len = dp->rec_len;

          // use memmove function to overwrite tempCP with the next cp
          // this shifts each dir to the left!
          memmove(tempCP, cp, cur_rec_len);

          // if cp + cur_len is > than buf+1024, then cp and dp is one dir past the last dir
          if(cp + cur_rec_len >= buf + BLKSIZE)
          {
            // set the dp to tempCP (AKA THE last dir)
            dp = (DIR *)tempCP;
            // add the original dir (that was removed) and add it to
            dp->rec_len += original_len;
            // break loop
            break;
          }
          // increment tempCP (we do this after the last dir check so when we
          //  call memmove(tempCP, cp) tempCP is always one dir behind cp)
          tempCP += dp->rec_len;
        }
        // write parent block back to disk
        put_block(tempParent->dev, tempParent->INODE.i_block[i], buf);
        tempParent->dirty = 1;
        return 1;
      }

      // increment all the char* and dir*s
      prev_rec_len = dp->rec_len;
      cp += dp->rec_len;
      tempCP += dp->rec_len;
      dp = (DIR *)cp;
    }
    printf("Error, there is no Directory!!!\n");
    return -1;
  }
}

int remove_dir()
{
  MINODE *mip, *pip;
  int pino;
  char parent[64], child[32], temp[64], linkName[128];
  DIR *dp;
  char *cp;
  char buf[BLKSIZE];

  // get parent path and child path
  strcpy(temp, pathname);
  strcpy(parent, dirname(temp));
  strcpy(temp, pathname);
  strcpy(child, basename(temp));

  // get pathname ino and set dev
  int ino = getino(dev, pathname);
  // get pathnames MINODE
  mip = iget(dev, ino);

  if(!S_ISDIR(mip->INODE.i_mode)) //check to see if it actually is a dir
  {
    printf("Invalid pathname, not a dir!\n");
    return -1;
  }

  // Dir might not be empty, need to check for FILES
  if(mip->INODE.i_links_count == 2)
  {
    //check to see if block is not empty
    if(mip->INODE.i_block[0] == 1)
    {
      // its nots so get the block and begin to iterate over its DIR
      get_block(dev, mip->INODE.i_block[0], buf);
      cp = buf;
      dp = (DIR*)buf;

      while (cp < buf + BLKSIZE)
      {
        // copy over a temp name for the dp->name
        strncpy(linkName, dp->name, dp->name_len);
        linkName[dp->name_len] = 0;

        //For each new dp, check if linkName is not "." or ".."
        if (strcmp(linkName, ".") != 0 && strcmp(linkName, "..") != 0)
        {
          // if linkName is not them, then it is a file inside the dir
          printf("Directory is not empty, may have files!\n");
          printf("File: %s\n", linkName);
          return -1;
        }
        //increment cp and dp;
        cp += dp->rec_len;
        dp = (DIR*)cp;
      }
    }
  }
  // Dir is not empty
  if (mip->INODE.i_links_count > 2)
  {
    printf("DIR NOT EMPTY!\n");
    return -1;
  }
  // for all direct datablocks deallocate them if they arent empty
  for (int i = 0; i < 12; i++)
  {
    if (mip->INODE.i_block[i] == 0)
    {
      continue;
    }
    bdealloc(dev, mip->INODE.i_block[i]);
  }
  // dealloc the inode
  idealloc(mip->dev, mip->ino);
  // put mip back to disk
  iput(mip);

  // dirname() lib function sets null paths to "." so set it back to null
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

  // check if parent is null so set pino equal to cwd's ino
  if(strcmp(parent, "") == 0)
  {
    pino = running->cwd->ino;
  }
  else
  {
    // else call getino on the parent
    pino = getino(dev, parent);
  }
  printf("pino = %d\n", pino);
  pip = iget(dev, pino); //gets the parent node

  // call remove child on the child string and parent MINODE
  rm_child(pip, child);

  pip->INODE.i_links_count--;
  pip->dirty = 1;
  iput(pip);
  return 1;
}
