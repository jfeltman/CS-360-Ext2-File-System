

int cat()
{
  char mybuf[1024], dummy = 0;
  int n;
  int i = 0;

  fd = open_file();
  strcpy(link, "0");
  while (n = myread(fd, mybuf, 1024))
  {
    mybuf[n] = 0;
    while (i < n)
    {
      printf("%c", mybuf[i]);
      i++;
    }
    printf("\n");
  }

  sprintf(pathname, "%d", fd);
  close_file();


}
