/* it converts IBM PC/ LINUX/ UNIX TXT files to CBM SEQ files
   in any Linux/Unix/DOS/Windows environments,
   it supports long filenames
   (C) VIII-2005, I-2006, IV-2017 V. Lidovski under GNU GPL v2
   each visible symbol is converted and remained unique

compilation with GNU DJ DELORIE GCC (http://www.delorie.com/djgpp) 
under DOS/Windows:
  gcc -o txt2seq.exe txt2seq.c

with GNU GCC under Linux:
  gcc -o txt2seq txt2seq.c

but almost any C compiler can be used
*/

#include <stdio.h>
#include <string.h>

FILE *fi, *fo;
char buf[63*1024] = "C64File", fni[256], fno[256], cbm_fn[17],
      fc[25], rmode[3] = "r", wmode[3] = "w";

unsigned char ascii2petcii[128] = {
//untran: 5c - \, 60 - `, 7b -{, 7d - }, 7e - ~
// 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 13,  0,  0,  0,  0,  0,  //0
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  //10
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,  //20
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,  //30
  64, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,  //40
 112,113,114,115,116,117,118,119,120,121,122, 91,191, 93, 94,164,  //50
 188, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,  //60
  80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,179,125,171,163,  0   //70
};

int dosnames () {
    if (fi = fopen("a, 56[]=+","w")) {
        fclose(fi);
        remove("a, 56[]=+");
        return 0;
    }
    return 1;
}


int linuxfs () {
    int st = 1;
    fi = fopen(".\\754xyz@).$(","w");
    fclose(fi);
    if (fi = fopen("754xyz@).$(","r")) {
        fclose(fi);
        st = 0;
    }
    remove(".\\754xyz@).$(");
    return st;
}

int main(int argc, char *argv[]) {
  int i, k, k1, n, dos, nix, seq = 0, upper = 0, dosfn = 0, keepext;

  if (argc >= 3) {
    seq = strchr(argv[2], '1') != 0;
    dosfn = strchr(argv[2], '2') != 0;
    upper = strpbrk(argv[2], "uU") != 0;
    keepext = strpbrk(argv[2], "kK") != 0;
    if (argc >= 4) {
      seq |= strchr(argv[3], '1') != 0;
      dosfn |= strchr(argv[3], '2') != 0;
      upper |= strpbrk(argv[3], "uU") != 0;
      keepext |= strpbrk(argv[3], "kK") != 0;
      if (argc == 5) {
        seq |= strchr(argv[4], '1') != 0;
        dosfn |= strchr(argv[4], '2') != 0;
        upper |= strpbrk(argv[4], "uU") != 0;
        keepext |= strpbrk(argv[4], "kK") != 0;
      }
    }
  }
  dosfn |= dos = dosnames();
  nix = linuxfs();

  printf("DR CP/M, DOS, Microsoft Windows, Linux/Unix text to\n");
  printf("CBM sequential file converter v1.1\nby V.Lidovski, VIII-2005, I-2006, IV-2017.\n");
  printf("This software is under GNU GPL v2.\n");
  printf("Files names status: ");
  if (nix)
    printf("Linux or something like it\n");
  else if (dos)
    printf("DOS or CP/M\n");
  else
    printf("Microsoft Windows\n");

  if (argc < 2 || argc > 5 || argc > 2 && !seq && !upper && !dosfn && !keepext) {
    printf("USAGE: txt2seq textfile [mode] [uppercase] [extension]\n");
    printf("  mode=0 (default) creates Snn file\n");
    printf("  mode=1 creates pure SEQ/TXT file - doesn't create header\n");
    printf("  mode=2 creates Snn file with 8.3 DOS filename\n");
    printf("  uppercase=u converts filename letters to CBM uppercase\n");
    printf("  extension=k keeps extension with CBM filename\n");
    return 1;
  }

  strcpy(fni, argv[1]);
  k = strlen(fni) - 1;
  while (k > 0 && fni[k] != '.') k--;
  if (k == 0 || keepext) {
    strcpy(fno, fni);
    k = strlen(fno);
  }
  else {
    strncpy(fno, fni, k);
    fno[k] = 0;
  }
  k1 = k;
  if (k > 8 && dosfn) {
      fno[7] = fno[k - 1];
      fno[k = 8] = 0;
  }
  if (seq)
    strcat(fno, ".seq");
  else {
    strcat(fno, ".s00");
    for (i = 0; i < 10; i++)
      if (fi = fopen(fno, "r")) {
        fclose(fi);
        fno[strlen(fno) - 1]++;
      }
      else
        break;
  }

  i = 0;
  while (i < k1 && i < 16) {
      if (!(cbm_fn[i] = ascii2petcii[fni[i]]))
          cbm_fn[i] = ' ';
      if (upper && fni[i] >= 'A' && fni[i] <= 'Z')
        cbm_fn[i] -= 32;
      i++;
  }
  while (i < 17) 
    cbm_fn[i++] = 0xA0;

  strcpy(buf + 8, cbm_fn);

  buf[25] = 0;
  buf[26] = 9;  //enable case switch
  buf[27] = 14; //cbm switch to lower case code

  if (nix) {
    strcpy(fc,"/");
    if (dosfn)
      strcat(fc," \"+*,.<=>:;?[\\]|");
  }
  else {
    strcat(rmode,"b");
    strcat(wmode,"b");
    strcpy(fc,"*/<>:?\\|");
    if (dosfn)
      strcat(fc," \"+,.=;[]");
  }

  i = 0;
  while (i < strlen(fno) - 4) {
    if (strchr(fc, fno[i]) != 0)
      fno[i] = '_';
    i++;
  }

  if (!(fi = fopen(fni, rmode))) {
        printf("%s not found\n", fni);
        return 2;
  }
  if (!(fo = fopen(fno, wmode))) {
        printf("Can't open %s\n", fno);
        return 3;
  }

  if (!seq)
    fwrite(buf, 1, 28, fo);
  i = fread(buf, 1, 63*1024, fi);
  while (i > 0) {
    n = 0;
    for (k = 0; k < i; k++)
      if (buf[n] = ascii2petcii[buf[k]])
          n++;
    if (!buf[n - 1]) n--;
    fwrite(buf, 1, n, fo);
    n = buf[n - 1];
    i = fread(buf, 1, 63*1024, fi);
  }
  buf[0] = 0xd;
  if (!seq && n != 0xd)
    fwrite(buf, 1, 1, fo);

  fclose(fo);
  fclose(fi);

  return 0;
}

