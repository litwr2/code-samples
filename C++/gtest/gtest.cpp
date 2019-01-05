#include <iostream>
#include <cmath>
using namespace std;
#include "list.h"
#include "graph.h"
#define dt 0.002

List <Point*> grl;

map <string, int> colorsmap;

char colors [][12] = {"black", "white", "red", "green", "blue", "brown",
  "cyan", "magenta", "yellow", "darkgreen", "lightgreen", "purple", "orange"};

struct Initcolors {
  Initcolors () {
    for (int i = 0; i < sizeof(colors)/12; i++)
      colorsmap [colors[i]] = i;
  }
} initcolors;

double x2 (double t, int sign) {
  return sign*(50*dt - (int(50*(t+dt)/XSIZE)-int(50*t/XSIZE))*XSIZE);
}

double x1 (double t, int sign) {
  return sign*-XSIZE*sin(t)*dt;
}

double x3 (double t, int sign) {
  return XSIZE/400.*sign;
}

double y3 (double t, int sign) {
  return YSIZE/400.*sign;
}

double y2 (double t, int sign) {
  return sign*-100*cos(t)*dt;
}

Circle circ(x1, y2, 40, 120, 10, "red");
Rectangle rect(x2, y2, 215,115, 140,140, "purple");
Ring ring(x1, x1, 200, 100, 30, 15, "green");
Letter lett(x3, y3, 'A', 0, YSIZE/2, "brown");
CircLett cl(x1, y2, 730, 200, 50, "magenta", 'B', "blue");

Point* mobs [] = {&circ, &rect, &ring, &lett, (Circle*) &cl, 0};

main () {
  double t = 0;
  cout << "area " << XSIZE << ' ' << YSIZE << ' ' << colors[BG] << endl;
  for (int i = 0; i < sizeof(mobs)/sizeof(Point*); i++)
    grl.insert(mobs[i]);
  grl.tofirst();
  do {
    grl.getdata()->toggle();
    grl.tonext();
  }
  while (!grl.eol());

  while (1) {
    grl.tofirst();
    do {
      grl.getdata()->move (
            (*grl.getdata()->xt)(t, grl.getdata()->xsign),
	    (*grl.getdata()->yt)(t, grl.getdata()->ysign));
      grl.tonext();
    }
    while(!grl.eol());
    t += dt;
  }
}
