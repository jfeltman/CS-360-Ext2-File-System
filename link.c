int mylink()
{
  MINODE *mip, *pip;
  char oparent[128], oldfile[128], nparent[128], newfile[128], temp[128];

  int ino = getino(dev, pathname); // get the ino of the file we want to link to

  if(ino == 0)
  {
    printf("wrong pathname!\n");
    return -1;
  }
  mip = iget(dev, ino); // points at inode of file we linking to

  //  we have 2 pathnames (pathname and link)
  strcpy(temp, pathname);
  strcpy(oparent, dirname(temp)); // oparent is the parent dir of pathname
  strcpy(temp, pathname);
  strcpy(oldfile, basename(temp)); // oldfile is the child file of pathname

  strcpy(temp, link);
  strcpy(nparent, dirname(temp)); // nparent is the parent dir of link
  strcpy(temp, link);
  strcpy(newfile, basename(temp)); // newfile is the childe file of link

  // chcek if the file is a reg/link and not a dir
  if(S_ISDIR(mip->INODE.i_mode))
  {
    printf("Cannot Link to filetype DIR\n");
    iput(mip);
    return -1;
  }

  // get the parent ino of the old parent dir
  int pino = getino(dev, nparent);
  pip = iget(dev, pino); // parent minode

  // make sure the newfile doesnt already exist
  if (kcwsearch(pip, newfile) != 0)
  {
    printf("File exists!\n");
    return -1;
  }
  iput(pip); // put back parent minodes

  link_create(ino, pino, newfile);

  mip->INODE.i_links_count++; //increment links count by 1

  iput(mip); // put back mip

  // int test_ino = getino(dev, link);
  // printf("%s ino number: %d\n", oldfile, test_ino);
  // printf("%s ino number: %d\n", newfile, ino);
}

int link_create(int ino, int pino, char *newfile)
{
  MINODE *mip;

  printf("pino = %d\n", pino);
  // get parent minode
  MINODE *pip = iget(dev, pino);

  printf("pip->ino = %d\n", pip->ino);

  // make sure parent minode is a dir and not a file
  if(!S_ISDIR(pip->INODE.i_mode))
  {
    printf("Invalid pathname, not a dir!\n");
    iput(pip);
    return;
  }

  myLink_creat(pip, newfile, ino);

  // touch time and dirty
  pip->INODE.i_atime = time(0L);
  pip->dirty = 1;

  // put back parent minode
  iput(pip);
}

int myLink_creat(MINODE *pip, char *name, int ino)
{
  MINODE *mip;

  // allocate a new inode number
  int newIno = ialloc(dev);

  // have mip point at minode pointer of new inode
  mip = iget(dev, newIno);
  INODE *ip = &mip->INODE; // ip is mip->INODE

  ip->i_mode = 0x81A4; // file
  ip->i_uid  = running->uid;	// Owner uid
  ip->i_gid  = running->gid;	// Group Id
  ip->i_size = 0;		// Size in bytes
  ip->i_links_count = 1;
  ip->i_atime = time(0L);  // set to current time
  ip->i_ctime = time(0L);
  ip->i_mtime = time(0L);
  ip->i_blocks = 2;                	// LINUX: Blocks count in 512-byte chunks
  for(int i = 0; i < 15; i++)
    ip->i_block[i] = 0;

  mip->dirty = 1;
  iput(mip); // put back mip

  enter_name(pip, ino, name);
}

int mySymLink()
{
  MINODE *mip, *pip;
  char newParent[64], newFile[60], temp[128];
  int pathLen;

  // get ino of pathname
  int ino = getino(dev, pathname);
  if(ino == 0) // check if path is right
  {
    printf("wrong pathname!\n");
    return -1;
  }
  mip = iget(dev, ino); // get minode of pathname

  // check to make sure that the pathname is a dir or a reg file
  if(!S_ISREG(mip->INODE.i_mode) && (!S_ISDIR(mip->INODE.i_mode)))
  {
    printf("Error! Pathname needs to be a file or dir!\n");
    iput(mip);
    return -1;
  }
  iput(mip); // write mip back to disk

  strcpy(temp, link);
  strcpy(newParent, dirname(temp)); // 2nd arg directory
  strcpy(temp, link);
  strcpy(newFile, basename(temp)); // 2nd arg child

  // get parent MINODE of the 2nd argument
  int pino = getino(dev, newParent);
  pip = iget(dev, pino);

  // make sure the path is a directory
  if(!S_ISDIR(pip->INODE.i_mode))
  {
    printf("2nd parameter path is not a directory!\n");
    iput(pip);
    return -1;
  }

  // make sure file doesnt already exist
  if(kcwsearch(pip, newFile) != 0)
  {
    printf("error file already exists!\n");
    iput(pip);
    return -1;
  }

  // creat the new file in parent directory
  my_creat(pip, newFile);

  // touch pip and write it back to disk
  pip->INODE.i_atime = time(0L);
  pip->dirty = 1;
  iput(pip);

  // now get ino of newfile we just created and get is minode pointer
  int newIno = getino(dev, link);
  mip = iget(dev, newIno);

  pathLen = strlen(pathname);

  mip->INODE.i_mode = 0xA1FF; // set mode to link (LNK)
  // copy pathname into inodes i_block
  strncpy((char *)mip->INODE.i_block, pathname, pathLen);
  mip->INODE.i_size = pathLen; // set size to len of pathnmae

  iput(mip); //write back to disk
}

// not sure if we need to make this a command in main, but it works
char *readlink(char *path)
{
  MINODE *mip;
  int ino = getino(dev, path);
  char *content;

  if(ino == 0)
  {
    printf("error, wrong path!\n");
    return -1;
  }

  mip = iget(dev, ino);

  if(!S_ISLNK(mip->INODE.i_mode))
  {
    printf("Error, not a symlink file!\n");
    iput(mip);
    return -1;
  }

  content = (char *)mip->INODE.i_block;
  iput(mip);

  return content;
}
