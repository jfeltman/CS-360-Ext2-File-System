void creat_file()
{
  MINODE *mip;
  int pino;

  char parent[64], child[32], temp[64];

  strcpy(temp, pathname);
  strcpy(parent, dirname(temp));
  strcpy(temp, pathname);
  strcpy(child, basename(temp));

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

  pino = getino(dev, parent);
  printf("pino = %d\n", pino);
  MINODE *pip = iget(dev, pino);

  printf("pip->ino = %d\n", pip->ino);

  if(!S_ISDIR(pip->INODE.i_mode))
  {
    printf("Invalid pathname, not a dir!\n");
    iput(pip);
    return;
  }
  if(kcwsearch(pip, child) != 0)
  {
    printf("Error, File already exists!\n");
    iput(pip);
    return;
  }
  my_creat(pip, child);

  pip->INODE.i_atime = time(0L);
  pip->dirty = 1;

  iput(pip);
}

int my_creat(MINODE *pip, char *name)
{
    MINODE *mip;

    int ino = ialloc(dev);

    mip = iget(dev, ino);
    INODE *ip = &mip->INODE;

    ip->i_mode = 0x81A4; // file type - REG FILE
    ip->i_uid  = running->uid;	// Owner uid
    ip->i_gid  = running->gid;	// Group Id
    ip->i_size = 0;		// Size in bytes
    ip->i_links_count = 1;	        // Links count=2 because of . and ..
    ip->i_atime = time(0L);  // set to current time
    ip->i_ctime = time(0L);
    ip->i_mtime = time(0L);
    ip->i_blocks = 2;                	// LINUX: Blocks count in 512-byte chunks
    for(int i = 0; i < 15; i++)
      ip->i_block[i] = 0;

    mip->dirty = 1;
    iput(mip);

    enter_name(pip, ino, name);
}
