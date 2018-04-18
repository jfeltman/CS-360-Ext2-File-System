

int mylink()
{
  MINODE *mip, *lip;
  INODE *ilink;
  char oparent[128], oldfile[128], nparent[128], newfile[128], temp[128];
  int ino = getino(dev, pathname);
  printf("C ino: %d\n", ino);

  mip = iget(dev, ino);
  printf("%s\n", pathname);
  printf("%s\n", link);
  tokenize(pathname);

  strcpy(temp, pathname);
  strcpy(oparent, dirname(temp));
  strcpy(temp, pathname);
  strcpy(oldfile, basename(temp));
  strcpy(temp, pathname);
  strcpy(temp, link);
  strcpy(nparent, dirname(temp));
  strcpy(temp, link);
  strcpy(newfile, basename(temp));

  printf("HERE\n");
  if(S_ISDIR(mip->INODE.i_mode))
  {
    printf("Cannot Link to filetype DIR\n");
    return -1;
  }

  int lino = getino(dev, nparent);

  lip = iget(dev, lino);

  if (kcwsearch(lip, newfile) != 0)
  {
    printf("File exists!\n");
    return -1;
  }

  link_create(ino);

  int test_ino = getino(dev, link);
  printf("%s ino number: %d\n", oldfile, test_ino);
  printf("%s ino number: %d\n", newfile, ino);


}

int link_create(int lino)
{
  MINODE *mip;
  int pino;

  char parent[64], child[32], temp[64];

  strcpy(temp, link);
  strcpy(parent, dirname(temp));
  strcpy(temp, link);
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
    printf("Error, File already exists!\n");
    iput(pip);
    return;
  }
  myLink_creat(pip, child, lino);

  pip->INODE.i_atime = time(0L);
  pip->dirty = 1;

  iput(pip);
}

int myLink_creat(MINODE *pip, char *name, int lino)
{
  MINODE *mip;
  char buf[BLKSIZE];
  DIR *dp;
  char *cp;

  int ino = ialloc(dev);

  mip = iget(dev, ino);
  INODE *ip = &mip->INODE;

  ip->i_mode = 0x81A4;
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

  dp = (DIR *)buf;
  cp = buf;

  enter_name(pip, lino, name);
}
