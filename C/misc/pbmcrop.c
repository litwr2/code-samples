/* crop pbm picture */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define sgn(x) (x?1:0)
#define MAXINT 32767

unsigned char overwrite = 0, count = 1, edgesst = 0, edgesu = 0, buf[1024],
  xs, xm, *infile, *outfile, *pstr, tmpout[] = "pbm$crop.tmp", crop = 1,
  force = 0;
long mp;
enum DIR {t, b, l, r};
int edges[] = {0,0,0,0}, x, y, xb, xn, xnb, xl, i, j,
  ae[] = {MAXINT, MAXINT, MAXINT, MAXINT};
FILE *fi = NULL, *fo = NULL;

sbl (int n) {
  int i = 7;
  while ((n >>= 1) != 0)
    i--;
  return i;
}

sbr (int n) {
  int i = 1;
  while (((n <<= 1) & 0xff) != 0)
    i++;
  return i;
}

findedges(void) {
  int i, j, k = 0, hst, vst = 1, ri, le;
  do {
    le = fread(buf, 1, xb, fi);
    if (le != xb)
      if (!le)
	break;
      else
	return 1;
    hst = 1;
    ri = MAXINT;
    le = MAXINT;
    for (i = 0; i < xb; i++) {
      j = buf[i];
      if (j && hst) {
	le = (i << 3) + sbl(j);
	hst = 0;
      }
      if (j && !hst)
	ri = x - (sbr(j) + (i << 3));
    }
    if (le < ae[l]) ae[l] = le;
    if (ri < ae[r]) ae[r] = ri;
    if (le != MAXINT && vst) {
      ae[t] = k;
      vst = 0;
    }
    if (le != MAXINT && !vst)
      ae[b] = y - k - 1;
    k++;
  } while (!feof(fi));
  for (i = 0; i < 4; i++)
    if ((edgesst & (1 << i)) != 0)
      edges[i] = ae[i];
  if (fseek(fi, mp, SEEK_SET))
    return 1;
  return 0;
}

getmetrics(void) {
  char s[128];
  if (fgets(s, 128, fi) == NULL)
    return 1;
  do {
    if (fgets(s, 128, fi) == NULL)
      return 1;
  } while (s[0] == '#');
  sscanf(s, "%d%d", &x, &y);
  if ((x <= 0) || (y <= 0))
    return 1;
  xb = x/8 + sgn(x%8);
  mp = ftell(fi);
  return 0;
}

void error(int n) {
  switch (n) {
    case 1:
      printf("PBMCROP 1.00 FREEWARE Copyright (C) VII-1999 Lidovski V.\n");
      printf("USAGE: pbmcrop -<switch> [-<switch> ...] inputfile [outputfile]\n");
      printf("<switch>:\n  bNN:  crop NN pixels from bottom\n");
      printf("  tNN:  crop NN pixels from top\n");
      printf("  rNN:  crop NN pixels from right\n");
      printf("  lNN:  crop NN pixels from left\n");
      printf("      instead NN may be used A (automatic search for edge)\n");
      printf("    i:  get information\n");
      printf("    f:  force crop\n");
      printf("Example: pbmcrop -bAl12 -t100 in.pbm out.pbm\n");
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
      break;
    case 7:
      fclose(fi);
      fclose(fo);
      unlink(outfile);
      printf("impossible parameter value\n");
  }
  exit(n);
}

void getparamn(int n) {
  int i = 1 << n;
  if ((edgesu & i) != 0)
    error(1);
  edgesu += i;
  switch (pstr[2]) {
    case 'A':
      edgesst += i;
      if (strlen(pstr) > 3)
	strcpy(pstr + 1, pstr + 3);
      else
	count++;
      break;
    default:
      i = 2;
      while (pstr[i] >= '0' && pstr[i] <= '9')
	edges[n] = edges[n]*10 + pstr[i++] - '0';
      if (i == 2)
	error(1);
      if (i < strlen(pstr))
	strcpy(pstr + 1, pstr + i);
      else
	count++;
   }
}

void getparam(void) {
  if (strlen(pstr) > 2)
     strcpy(pstr + 1, pstr + 2);
   else
     count++;
}

void overopen(void) {
  overwrite = 1;
  /* fprintf(stderr, "overwrite mode\n"); */
  if (crop)
    fo = fopen(outfile = tmpout, "wb");
}

void getparams(int argc, char *argv[]) {
  if (argc < 3)
    error(1);
  while (count < argc) {
    pstr = argv[count];
    if (pstr[0] == '-')
      switch (pstr[1]) {
	case 'b':
	  getparamn(b);
	  break;
	case 't':
          getparamn(t);
	  break;
	case 'l':
          getparamn(l);
	  break;
	case 'r':
	  getparamn(r);
	  break;
	case 'i':
	  getparam();
	  crop = 0;
	  edgesst = 15;
	  break;
	case 'f':
	  getparam();
	  force = 1;
	  break;
	default:
	  error(5);
      }
    else
      if (fi == NULL)
        if ((fi = fopen(infile = argv[count++], "rb")) == NULL)
          error(2);
        else if (count == argc)
          overopen();
      else
        continue;
      else if (!strcmp(infile, argv[count]))
        overopen();
      else if (fo != NULL)
        error(1);
      else if (crop && ((fo = fopen(outfile = argv[count], "wb")) == NULL))
        error(3);
      else
        count++;
  }
  if (fi == NULL || crop && fo == NULL)
    error(1);
}
        
main(int argc, char *argv[]) {
  getparams(argc, argv);
  if (getmetrics())
    error(4);
  if (!force || !crop)
    if (findedges())
      error(4);
  if (!crop) {
    /* fprintf(stderr, " width height    top bottom   left  right\n"); */
    printf("%6d %6d %6d %6d %6d %6d\n", x, y, ae[t], ae[b], ae[l], ae[r]);
    goto l1;
  }
  if (edges[t] + edges[b] >= y || edges[l] + edges[r] >= x)
    error(7);
  if (edgesst != 15 && !force)
    for (i = 0; i < 4; i++)
      if (edges[i] > ae[i])
        error(7);
  xn = x - edges[l] - edges[r];
  y -= edges[t] + edges[b];
  fprintf(fo, "P4\n%d %d\n", xn, y);
  xs = edges[l] % 8;
  xl = edges[l]/8;
  xnb = xn/8 + sgn(xn%8);
  while (edges[t]-- > 0)
    fread(buf, 1, xb, fi);
  while (y-- > 0) {
    fread(buf, 1, xb, fi);
    for (i = 0, j = xl; i < xnb; i++, j++)
      buf[j] = ((buf[j + 1] + (((unsigned) buf[j]) << 8)) << xs) >> 8;
    xm = xn % 8;
    if (xm)
      buf[xnb + xl - 1] &= 256 - (2 << (7 - xm));
    fwrite(buf + xl, 1, xnb, fo);
  }
  fclose(fo);
  l1: fclose(fi);
  if (overwrite && crop) {
    unlink(infile);
    rename(tmpout, infile);
  }
  return 0;
}
