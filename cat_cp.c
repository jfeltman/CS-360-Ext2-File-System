int cat()
{
  char mybuf[BLKSIZE];
  int n, i = 0;
  // put link as 0 so when we call open it, opens with mode 0 (read)
  strcpy(link, "0");

  fd = open_file();
  // file doesnt exist
  if(fd == -1)
  {
    return -1;
  }

  printf("========= %s =========\n", pathname);
  while (n = myread(fd, mybuf, BLKSIZE))
  {
    mybuf[n] = 0;
    while (i < n)
    {
      printf("%c", mybuf[i]);
      i++;
    }
    printf("\n");
  }
  printf("=======================\n");
  // write fd to pathname so we can close the file
  sprintf(pathname, "%d", fd);
  close_file();
}

int my_cp()
{
  int n, gd;
  char src_file[128], dst_file[128], temp[128];
  char buf[BLKSIZE];

  strcpy(src_file, pathname);
  strcpy(dst_file, link);

  // if dst doesnt exist, creat it
  if(getino(dev, dst_file) == 0)
  {
    printf("No dest_file, creating new one!\n");
    strcpy(temp, pathname);
    strcpy(pathname, dst_file);
    creat_file(); // creat the empty dst file
    strcpy(pathname, temp); // reset path
  }

  // open src_file for read
  strcpy(link, "0");
  fd = open_file(); // opens src for read

  strcpy(pathname, dst_file);
  strcpy(link, "2");
  gd = open_file(); // opens dst for R/W

  // read in n bytes to buf, then write it to newfile
  while(n = myread(fd, buf, BLKSIZE))
  {
    myWrite(gd, buf, n);
  }

  // close the files
  strcpy(pathname, "0");
  close_file();
  strcpy(pathname, "1");
  close_file();
}

int mv()
{
  MINODE *mip;
  int ino;

  ino = getino(dev, pathname);
  if(ino == 0)
  {
    printf("Error, invalid path!\n");
    return -1;
  }
  mip = iget(dev, ino);

  // check if src dev == cur dev?
  if(mip->dev == running->cwd->dev)
  {
    mylink(); // link src and dst
    strcpy(link, "");
    myUnlink(); // unlink src
  }
  else
  {
    my_cp(); // cp src into dst
    strcpy(link, "");
    myUnlink(); // unlink src
  }

  iput(mip); // put back mip
}
