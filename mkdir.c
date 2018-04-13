int mymkdir(MINODE *pip, char *name)
{
  // pip points at parent inode, name is child we want to make

  MINODE *mip;
  char buf[BLKSIZE];
  DIR *dp;
  char *cp;

  int ino = ialloc(dev);
  int bno = balloc(dev);

  mip = iget(dev, ino);
  INODE *ip = &mip->INODE;

  ip->i_mode = 0x41ED;
  ip->i_uid  = running->uid;	// Owner uid
  ip->i_gid  = running->gid;	// Group Id
  ip->i_size = BLKSIZE;		// Size in bytes
  ip->i_links_count = 2;	        // Links count=2 because of . and ..
  ip->i_atime = time(0L);  // set to current time
  ip->i_ctime = time(0L);
  ip->i_mtime = time(0L);
  ip->i_blocks = 2;                	// LINUX: Blocks count in 512-byte chunks
  ip->i_block[0] = bno;             // new DIR has one data block
  for(int i = 1; i < 15; i++)
    ip->i_block[i] = 0;

  mip->dirty = 1;
  iput(mip);

  // Now write . and .. into buf
  dp = (DIR *)buf;
  dp->inode = ino; // . = ino
  strncpy(dp->name, ".", 1); // copy name
  dp->name_len = 1; // name length
  dp->rec_len = 12; // . has size of 12

  cp = buf; // set cp to buf
  cp += dp->rec_len; // increment cp by dp->rec_len (get next dir)
  dp = (DIR *)cp; // dp = cp now we at .. entry

  dp->inode = pip->ino; // .. inode is parents inodes
  strncpy(dp->name, "..", 2);
  dp->name_len = 2;
  dp->rec_len = 1012; // .. size is 1024 - (. size)

  put_block(dev, bno, buf); // write buf to disk block bno

  enter_name(pip, ino, name);

  //iput(pip);
}

void make_dir()
{
  MINODE *mip;
  int pino;

  char parent[64], child[32], temp[64];

  strcpy(temp, pathname);
  strcpy(parent, dirname(temp));
  strcpy(temp, pathname);
  strcpy(child, basename(temp));

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

  printf("parent - %s\n", parent);
  printf("child - %s\n", child);

  if(strcmp(parent, "") == 0)
  {
    pino = running->cwd->ino;
  }
  else
  {
    pino = getino(dev, parent);
  }
  printf("pino = %d\n", pino);
  MINODE *pip = iget(dev, pino);

  printf("pip->ino = %d\n", pip->ino);

  if(!S_ISDIR(pip->INODE.i_mode))
  {
    printf("Invalid pathname, not a dir!\n");
    iput(pip);
    return;
  }

  int check_exists = kcwsearch(pip, child);
  if(check_exists != 0)
  {
    printf("Error, Directory already exists!\n");
    iput(pip);
    return;
  }
  mymkdir(pip, child);

  pip->INODE.i_links_count += 1;
  pip->INODE.i_atime = time(0L);
  pip->dirty = 1;

  iput(pip);
}
