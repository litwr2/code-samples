/* catenate two pbm pictures */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define iceil(n) ((n)/8 + ((n)%8?1:0))
#define max(n,m) (((n)>(m))?(n):(m))

unsigned char count = 1, buf[2048], *infile, *outfile, *pstr;
enum DIR {h, v, u} dir = u;
int x[3] = {0,0,0}, y[3] = {0,0,0}, xb[3] = {0,0,0}, i, j, gap = 0, gapb;
FILE *fi[2] = {NULL, NULL}, *fo = NULL;

getmetrics(int n) {
  char s[128];
  if (fgets(s, 128, fi[n]) == NULL)
    return 1;
  do {
    if (fgets(s, 128, fi[n]) == NULL)
      return 1;
  } while (s[0] == '#');
  sscanf(s, "%d%d", &(x[n]), &(y[n]));
  if ((x[n] <= 0) || (y[n] <= 0))
    return 1;
  xb[n] = iceil(x[n]);
  return 0;
}

void error(int n) {
  switch (n) {
    case 1:
      printf("PBMCAT 1.00 FREEWARE Copyright (C) VII-1999 Lidovski V.\n");
      printf("USAGE: pbmcat -<switch> [-<switch> ...] inputfile1 inputfile2 outputfile\n");
      printf("<switch>:\n  v:  vertical catenation\n");
      printf("  h:  horisontal catenation\n");
      printf("  gNN:  add interpictures gap NN pixels width\n");
      printf("Example: pbmcat -vg150 pic1.pbm pic2.pbm pic3.pbm\n");
      break;
    case 2:
      printf("can't open %s for read\n", infile);
      break;
    case 3:
      printf("can't open %s for write\n", outfile);
      break;
    case 4:
      printf("error in pbm file\n");
      break;
    case 5:
      printf("unknown parameter found\n");
  }
  exit(n);
}

void getparamn(void) {
  int i = 2;
  while (pstr[i] >= '0' && pstr[i] <= '9')
    gap = gap*10 + pstr[i++] - '0';
  if (i == 2)
    error(1);
  if (i < strlen(pstr))
    strcpy(pstr + 1, pstr + i);
  else
    count++;
  gapb = iceil(gap);
}

void getparam(void) {
  if (strlen(pstr) > 2)
     strcpy(pstr + 1, pstr + 2);
   else
     count++;
}

void getparams(int argc, char *argv[]) {
  if (argc < 5 || argc > 6)
    error(1);
  while (count < argc) {
    pstr = argv[count];
    if (pstr[0] == '-')
      switch (pstr[1]) {
        case 'g':
          getparamn();
	  break;
        case 'v':
	  getparam();
          if (dir == h)
            error(1);
          else
            dir = v;
	  break;
        case 'h':
	  getparam();
          if (dir == v)
            error(1);
          else
            dir = h;
	  break;
	default:
	  error(5);
      }
    else
      if (fi[0] == NULL)
        if ((fi[0] = fopen(infile = argv[count++], "rb")) == NULL)
          error(2);
        else
          continue;
      else if (fi[1] == NULL)
        if ((fi[1] = fopen(infile = argv[count++], "rb")) == NULL)
          error(2);
        else
          continue;
      else if (fo == NULL)
        if ((fo = fopen(outfile = argv[count++], "wb")) == NULL)
          error(3);
        else
          continue;
      else
        error(1);
  }
  if (fi[0] == NULL || fi[1] == NULL || fo == NULL || dir == u)
    error(1);
}

catenate(void) {
  int i, j, k, s, n, l, m;
  if (dir == v) {
      memset(buf, 0, xb[2]);
      while (!feof(fi[0])) {
	i = fread(buf, 1, xb[0], fi[0]);
	if (i != xb[0])
	  if (!i)
	    break;
	  else
	    return 1;
	fwrite(buf, 1, xb[2], fo);
      }
      memset(buf, 0, xb[2]);
      for (i = 0; i < gap; i++)
	fwrite(buf, 1, xb[2], fo);
      while (!feof(fi[1])) {
        i = fread(buf, 1, xb[1], fi[1]);
        if (i != xb[1])
          if (!i)
            break;
          else
	    return 1;
	fwrite(buf, 1, xb[2], fo);
      }
  } else {
    k = (x[0] + gap)/8;
    s = (x[0] + gap)%8;
    for (j = 0; j < y[2]; j++) {
      if (!feof(fi[0])) {
	i = fread(buf, 1, xb[0], fi[0]);
	if (i != xb[0])
	  if (i)
	    return 1;
	  else
            memset(buf, 0, xb[0]);
       } else
         memset(buf, 0, xb[0]);
       if (gap > 0)
         memset(buf + xb[0], 0, gapb);
       n = buf[k] & (0x100 - (1 << s));
       if (!feof(fi[1])) {
	i = fread(buf + k, 1, xb[1], fi[1]);
	if (i != xb[1])
	  if (i)
	    return 1;
	  else
            memset(buf + k, 0, xb[1]);
       } else
         memset(buf + k, 0, xb[1]);
       buf[k + xb[1]] = 0;
       for (m = xb[1], l = k + xb[1]; m > 0; m--, l--)
	 buf[l] = ((((unsigned) buf[l - 1] << 8) + buf[l]) >> s) & 0xff;
       buf[k] = (buf[k] >> s) & 0xff | n;
       fwrite(buf, 1, xb[2], fo);
    }
  }
  return 0;
}


main(int argc, char *argv[]) {
  getparams(argc, argv);
  if (getmetrics(0) || getmetrics(1))
    error(4);
  if (dir == v) {
    x[2] = max(x[0], x[1]);
    y[2] = y[0] + y[1] + gap;
  } else {
    y[2] = max(y[0], y[1]);
    x[2] = x[0] + x[1] + gap;
  }
  xb[2] = iceil(x[2]);
  printf("%dx%d + %dx%d = %dx%d\n", x[0], y[0], x[1], y[1], x[2], y[2]);
  fprintf(fo, "P4\n%d %d\n", x[2], y[2]);
  catenate();
  fclose(fo);
  fclose(fi[0]);
  fclose(fi[1]);
  return 0;
}

