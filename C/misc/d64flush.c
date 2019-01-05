/* it converts D64 image to the set of Xnn files
   in any Linux/Unix/DOS/Windows environments, it supports long filenames,
  (C) XII-2006, I,VI,VII-2012 V. Lidovski under GNU GPL v2.

compilation with GNU DJ DELORIE GCC (http://www.delorie.com/djgpp) 
under DOS/Windows:
  gcc -o d64flush.exe d64flush.c

with GNU GCC under Linux:
  gcc -o d64flush d64flush.c

but almost any C compiler can be used.
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>

FILE *fi, *fo, *ft;

unsigned char buf[63*1024], fno[256], fc[25], zf = 0, cbm_fn[17],
   id[] = "C64File", rmode[3] = "r", wmode[3] = "w", dos, dosfn = 0,
   nix, path[17], dironly, nofolder;

struct {
   unsigned char rl, ty, attr, fn[17], track, sect;
   int sz;
} cbm[144];

char petcii2ascii[256] = {
//untran:5c - pound, 60,7b,7c,7e,7f,a1-a3,a5-ae,b0-bf,ff; copy: e1-e3,...
// 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
   0,  0,  0,  0,  0,  0,  0,  7,  0,  9,  0,  0,  0, 10,  0,  0,  //0
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  //10
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,  //20
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,  //30
  64, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,  //40
 112,113,114,115,116,117,118,119,120,121,122, 91, 76, 93, 94, 95,  //50
  45, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,  //60
  80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,'+','{','|','#','#',  //70
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10,  0,  0,  //80
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  //90
 ' ','{','m','~','_','{','#','}','m','#','}','}','.','`','.','_',  //A0
 '.','~','w','{','{','{','}','~','~','_','V','.','`','`','~','#',  //B0
  45, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,  //C0
  80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,'+','{','|','#','#',  //D0
 ' ','{','m','~','_','{','#','}','m','#','}','}','.','`','.','_',  //E0
 '.','~','w','{','{','{','}','~','~','_','V','.','`','`','~','#'   //F0
};

int dosnames () {
   char sfn[] = "a, 56[]=+";
   if (fi = fopen(sfn, "w")) {
      fclose(fi);
      remove(sfn);
      return 0;
   }
   return 1;
}

int linuxfs () {
   char sfn[] = "/tmp/a.\\4::x@..$";
   int st = 0;
   fi = fopen(sfn, "w");
   if (fi)
      fclose(fi);
   else
      return 0;
   if (fi = fopen(sfn,"r")) {
      fclose(fi);
      st = 1;
   }
   remove(sfn);
   return st;
}

int seek_d64pos(int track, int sect) {
   long addr = 0;
   if (track < 1 || track > 40 || sect > 20)
      return -1;
   track--;
   if (track <= 17)
      addr = 21*track;
   else {
      addr = 21*17;
      track -= 17;
      if (track <= 7)
         addr += 19*track;
      else {
         addr += 19*7;
         track -= 7;
         if (track <= 6)
            addr += 18*track;
         else
            addr += 18*6 + 17*(track - 6);
      }
   }
   addr = (addr + sect)*256;
   fseek(fi, addr, SEEK_SET);
   return 1;
}

int fill_d64fl () {
   unsigned char nextsect = 1, nextrack = 18, idx = 0, dirp;
   int st, k1;

   do {
      if ((st = seek_d64pos(nextrack, nextsect)) < 0)
         break;
      dirp = 0;
      fread(buf, 1, 256, fi);
      nextrack = buf[0];
      nextsect = buf[1];
      do {
         if (buf[2 + dirp*32] == 0)
            continue;
         cbm[idx].rl = buf[23 + dirp*32];
         if ((buf[2 + dirp*32]&15) == 1)
            cbm[idx].ty = 's';
         else if ((buf[2 + dirp*32]&15) == 2)
            cbm[idx].ty = 'p';
         else if ((buf[2 + dirp*32]&15) == 3)
            cbm[idx].ty = 'u';
         else if ((buf[2 + dirp*32]&15) == 4)
            cbm[idx].ty = 'r';
         else
            cbm[idx].ty = 'd';
         if (buf[2 + dirp*32]&0x40)
            cbm[idx].attr = 'r';
         else
            cbm[idx].attr = ' ';
         cbm[idx].track = buf[3 + dirp*32];
         cbm[idx].sect = buf[4 + dirp*32];
         if (!(cbm[idx].sz = buf[30 + dirp*32] + buf[31 + dirp*32]*256) && !zf && !dironly)
            continue;
         strncpy(cbm[idx].fn, buf + 5 + dirp*32, 16);
         for (k1 = 16; k1 > 0; k1--) { 
            cbm[idx].fn[k1] = 0;
            if (cbm[idx].fn[k1-1] != 0xa0)
               break;
         }
         idx++;
      } while (++dirp < 8);
   } while (nextrack > 0 && idx < 144 && st > 0);
   return idx;
}

int readsect (unsigned char *track, unsigned char *sect) {
   if (seek_d64pos(*track, *sect) < 0)
      return 0;
   fread(buf, 1, 256, fi);
   *track = buf[0];
   *sect = buf[1];
   if (*track > 0)
      return 254;
   else
      return *sect - 1 < 0 ? 0 : *sect - 1;
}

void adjustfn(unsigned char *fno, int fixdos) {
   int k, n;
   for (k = strlen(fno) - 1; k > 0; k--)
      if (fno[k] != 0xa0)
         break;
   fno[++k] = 0;
   if (fixdos && dosfn && k > 8) {
      fno[7] = fno[k - 1];
      fno[k = 8] = 0;
   }
   if (k == 0)
      strcpy(fno, "_");
   n = 0;
   while (n < k) {
      if (!(fno[n] = petcii2ascii[fno[n]]))
         fno[n] = '_';
      n++;
   }
   k = 0;
   while (k < strlen(fno)) {
      if (strchr(fc, fno[k]) != 0)
         fno[k] = '_';
      k++;
   }
}

char *fixcase(char *s) {
   static char t[20];
   int i;
   strcpy(t, s);
   for (i = 0; i < strlen(t); i++)
      if (t[i] >= 'A' && t[i] <= 'Z')
         t[i] += 32;
      else if (t[i] >= 'a' && t[i] <= 'z')
         t[i] -= 32;
   return t;
}

main(int argc, char *argv[]) {
   int i, k, q, fr = 663;

   if (argc == 3) {
      zf = (strchr(argv[2], 'Z') || strchr(argv[2], 'z'));
      dosfn = (strchr(argv[2], 'D') || strchr(argv[2], 'd'));
      dironly = (strchr(argv[2], 'L') || strchr(argv[2], 'l'));
      nofolder = (strchr(argv[2], 'C') || strchr(argv[2], 'c'));
   }
   dosfn |= dos = dosnames();
   nix = linuxfs();
   if (!dironly) {
      printf("Multiplatform D64-image's files extractor v1.1\n");
      printf("by V.Lidovski, VII-2012. This software is under GNU GPL v2.\n");
      printf("Filenames status: ");
      if (nix)
         printf("Linux or something like it\n");
      else if (dos)
         printf("DOS or CP/M\n");
      else
         printf("Microsoft Windows\n");
   }
   if (argc < 2 || argc > 3) {
      printf("USAGE: d64flush D64file [mode]\n");
      printf("  mode is determinated by the sequence of letters\n");
      printf("    Z means to force the extraction of zero length files\n");
      printf("    D means to creates files with 8.3 DOS filenames\n");
      printf("    C means to extract files to the current folder\n");
      printf("    L means to list files\n");
      printf("For example,\n");
      printf("  d64flush disk.d64 CZD\n");
      printf("  d64flush game.d64 L\n");
      return 1;
   }

   if (nix) {
      strcpy(fc, "/");
      if (dosfn)
         strcat(fc, " \"+*,.<=>:;?[\\]|");
   }
   else {
      strcat(rmode, "b");
      strcat(wmode, "b");
      strcpy(fc, "*/<>:?\\|");
      if (dosfn)
         strcat(fc, " \"+,.=;[]");
   }

   i = strlen(argv[1]) - 4;
   if (strcmp(".d64", argv[1] + i) && strcmp(".D64", argv[1] + i)) {
      printf("Argument must be a D64 file!\n");
      return 2;
   }

   if ((fi = fopen(argv[1], rmode)) == 0) {
      printf("%s not found\n", argv[1]);
      return 3;
   }
   q = fill_d64fl();
   if (q > 0) {
      seek_d64pos(18, 0);
      fread(buf, 1, 256, fi);
      strncpy(path, buf + 144, 16);
      path[16] = 0;
      adjustfn(path, 1);
      strcat(path, ".");
      adjustfn(buf + 162, 1);
      buf[164] = 0;
      strcat(path, buf + 162);
      printf("diskname: %s\n", path);
      if (!dironly && !nofolder)
         if (mkdir(fixcase(path), 0777) < 0) {
            k = 0;
            for (i = 0; i < strlen(path); ++i)
               k += (unsigned)path[i];
            sprintf(path, "__baddir.%d", k%1000);
            if (mkdir(path, 0777) < 0) {
               printf("Can't create directory for the files\n");
               return 5;
            }
         }
   }

   printf("#          FN        TA  RL   T   S  SZ\n");
   for (i = 0; i < q; i++) {
      strcpy(fno, cbm[i].fn);
      adjustfn(fno, 0);
      printf("%2d. %-16s %c%c %3d %3d %3d %3d\n", i + 1, fno, cbm[i].ty,
                 cbm[i].attr, cbm[i].rl, cbm[i].track, cbm[i].sect, cbm[i].sz);
      strcpy(fno, cbm[i].fn);
      adjustfn(fno, 1);
      strcpy(buf, path);
      if (nix)
         strcat(buf, "/");
      else
         strcat(buf, "\\");
      if (nofolder)
         strcpy(buf, fno);
      else
         strcat(buf, fno);
      strcpy(fno, buf);
      strcat(fno, ".x00");
      fno[strlen(fno) - 3] = cbm[i].ty;
      for (k = 0; k < 10; k++)
         if (ft = fopen(fixcase(fno), "r")) {
            fclose(ft);
            fno[strlen(fno) - 1]++;
         }
         else
            break;

      strcpy(buf, id);
      strncpy(buf + 8, cbm[i].fn, 16);
      buf[24] = 0;
      buf[25] = cbm[i].rl;

      fr -= cbm[i].sz;
      if (!dironly) {
         if (!(fo = fopen(fixcase(fno), wmode))) {
            printf("ERROR! can't extract %s\n", fno);
            continue;
         }
         fwrite(buf, 1, 26, fo);
         do {
            k = readsect(&cbm[i].track, &cbm[i].sect);
            if (k > 0)
               fwrite(buf + 2, 1, k, fo);
         } while (k > 0);
         fclose(fo);
      }
   }
   printf("%7d blocks free\n", fr);
   fclose(fi);
}

