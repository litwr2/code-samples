#include <iostream>
#include <map>
#include <string>
using namespace std;
#include "graph.h"


Point :: Point (double (*xt0) (double, int), double (*yt0) (double, int),
         int x0 = 0, int y0 = 0, const char* c0 = colors[0]): 
	 x(x0), y(y0), color(colorsmap[c0]),
         vis(0), xsign(1), ysign(1), xt(xt0), yt(yt0) {q++;}

int Point :: q = 0;

void Point :: toggle () {
  vis = !vis;
  if (vis)
    cout << "point g" << (unsigned long) this << ' ' << x << ' ' << y << ' '
       << colors [color] << endl;
  else
    cout << "delete g" << (unsigned long) this << endl;
}

void Point :: move (double dx, double dy) {
  x += dx;
  y += dy;
  if (x < 0 || x > XSIZE)
    xsign = -xsign;
  else if (y < 0 || y > YSIZE)
    ysign = -ysign;
  if (vis)
    cout << "move g" << (unsigned long) this << ' ' << dx << ' ' << dy << endl;

}

Circle :: Circle (double (*xt0) (double, int), double (*yt0) (double, int),
        int x0 = 0, int y0 = 0, int r0 = 1, const char* c0 = colors[0]):
                      Point(xt0, yt0, x0, y0, c0), radius(r0) {}

void  Circle :: toggle () {
  vis = !vis;
  if (vis)
    cout << "circle g" << (unsigned long) this << ' ' << x << ' ' << y
      << ' ' << radius << ' ' << colors [color] << endl;
  else
    cout << "delete g" << (unsigned long) this << endl;
}

void Circle :: changeColor (int c) {
  color = c;
  if (vis)
    cout << "delete g" << (unsigned long) this << endl << "circle g"
         << (unsigned long) this << ' ' << x << ' ' << y
         << ' ' << radius << ' ' << colors [color] << endl;
}

Rectangle :: Rectangle (double (*xt0) (double, int), double (*yt0) (double, int),
             int x0 = 0, int y0 = 0, int xb0 = 1, int yb0 = 1, 
	     const char* c0 = colors[0]):
                              Point(xt0, yt0, x0, y0, c0), xb(xb0), yb(yb0) {}

void Rectangle :: toggle () {
  vis = !vis;
  if (vis)
    cout << "line g" << (unsigned long) this << ' ' << x << ' ' << y << ' '
         << xb << ' ' << y << ' ' <<colors [color] << endl
         << "line g" << (unsigned long) this + 1 << ' ' << xb << ' ' << y
         << ' ' << xb << ' ' << yb << ' ' << colors [color] << endl
         << "line g" << (unsigned long) this + 2 << ' ' << x << ' ' << y
         << ' ' << x << ' ' << yb << ' ' << colors [color] << endl
         << "line g" << (unsigned long) this + 3 << ' ' << x << ' ' << yb
         << ' ' << xb << ' ' << yb << ' ' << colors [color] << endl;

  else
    cout << "delete g" << (unsigned long) this << endl
         << "delete g" << (unsigned long) this + 1 << endl
         << "delete g" << (unsigned long) this + 2 << endl
         << "delete g" << (unsigned long) this + 3 << endl;
}


void Rectangle :: changeColor (int c) {
  color = c;
  if (vis) 
    cout << "delete g" << (unsigned long) this << endl << "line g"
         << (unsigned long) this << ' ' << x << ' ' << y
         << ' ' << xb << ' ' << y << ' ' << colors [color] << endl
         << "delete g" << (unsigned long) this + 1 << endl << "line g"
         << (unsigned long) this + 1 << ' ' << xb << ' ' << y
         << ' ' << xb << ' ' << yb << ' ' << colors [color] << endl
         << "delete g" << (unsigned long) this + 2 << endl << "line g"
         << (unsigned long) this + 2 << ' ' << x << ' ' << y
         << ' ' << x << ' ' << yb << ' ' << colors [color] << endl
         << "delete g" << (unsigned long) this + 3 << endl << "line g"
         << (unsigned long) this + 3 << ' ' << x << ' ' << yb
         << ' ' << xb << ' ' << yb << ' ' << colors [color] << endl;
}

void Rectangle :: move (double dx, double dy) {
  Point :: move(dx, dy);
  if (vis)
    cout << "move g" << (unsigned long) this + 1 << ' ' << dx << ' ' << dy << endl
         << "move g" << (unsigned long) this + 2 << ' ' << dx << ' ' << dy << endl
         << "move g" << (unsigned long) this + 3 << ' ' << dx << ' ' << dy << endl;
}

Ring :: Ring (double (*xt0) (double, int), double (*yt0) (double, int),
        int x0 = 0, int y0 = 0, int r0 = 2, int ir0 = 1, 
	const char* c0 = colors[0]):
             Circle(xt0, yt0, x0, y0, r0, c0), ir(ir0) {}

void Ring :: toggle () {
  vis = !vis;
  if (vis)
    cout << "circle g" << (unsigned long) this << ' ' << x << ' ' << y
      << ' ' << getRadius() << ' ' << colors [color] << endl
      << "circle g" << (unsigned long) this + 1 << ' ' << x << ' ' << y
      << ' ' << ir << ' ' << colors [BG] << endl;
  else
    cout << "delete g" << (unsigned long) this << endl << "delete g"
         << (unsigned long) this + 1 << endl;
}


void Ring :: changeColor (int c) {
  color = c;
  if (vis)
    cout << "delete g" << (unsigned long) this << endl << "circle g"
         << (unsigned long) this << ' ' << x << ' ' << y
         << ' ' << getRadius() << ' ' << colors [color] << endl
	 << "delete g" << (unsigned long) this + 1 << endl << "circle g"
         << (unsigned long) this + 1 << ' ' << x << ' ' << y
         << ' ' << ir << ' ' << colors [BG] << endl;
}

void Ring :: move (double dx, double dy) {
  Point :: move(dx, dy);
  if (vis)
    cout << "move g" << (unsigned long) this + 1 << ' ' << dx << ' ' << dy << endl;
}

Letter :: Letter (double (*xt0) (double, int), double (*yt0) (double, int),
           char ch0, int x0 = 0, int y0 = 0, const char* c0 = colors[0]):
                  Point(xt0, yt0, x0, y0, c0), ch(ch0) {}

void Letter :: toggle () {
  vis = !vis;
  if (vis)
    cout << "text g" << (unsigned long) this << ' ' << x << ' ' << y
      << ' ' << colors [color] << ' ' << ch << endl;
  else
    cout << "delete g" << (unsigned long) this << endl;
}


void Letter :: changeColor (int c) {
  color = c;
  if (vis)
    cout << "delete g" << (unsigned long) this << endl << "text g"
         << (unsigned long) this << ' ' << x << ' ' << y
         << ' ' << colors [color] << ' ' << ch << endl;
}

CircLett :: CircLett (double (*xt0) (double, int), double (*yt0) (double, int),
              int x0 = 0, int y0 = 0, int r0 = 1, const char* c0 = colors[0],
		     char ch0 = 'A', const char* c1 = colors[8]):
        Circle (xt0, yt0, x0, y0, r0, c0), Letter(xt0, yt0, ch0, x0, y0, c1) {}
	
void CircLett :: toggle () {
  Circle::vis = !Circle::vis;
  if (Circle::vis)
    cout << "circle g" << (unsigned long) this << ' ' << Circle::x 
      << ' ' << Circle::y
      << ' ' << getRadius() << ' ' << colors [Circle::color] << endl
      << "text g" << (unsigned long) this + 1 << ' ' << Circle::x 
      << ' ' << Circle::y
      << ' ' << colors [Letter::color] << ' ' << ch << endl;
  else
    cout << "delete g" << (unsigned long) this << endl << "delete g"
         << (unsigned long) this + 1 << endl;
}

void CircLett :: changeColor (int c1, int c2 = 1) {
  Circle::color = c1;
  Letter::color = c2;
  if (Circle::vis)
    cout << "delete g" << (unsigned long) this << endl << "circle g"
         << (unsigned long) this << ' ' << Circle::x << ' ' << Circle::y
         << ' ' << getRadius() << ' ' << colors [Circle::color] << endl
	 << "delete g" << (unsigned long) this + 1 << endl << "letter g"
         << (unsigned long) this + 1 << ' ' << Circle::x << ' ' << Circle::y
         << ' ' << ch << ' ' << colors [Letter::color] << endl;
}

void CircLett :: move (double dx, double dy) {
  if (Circle::vis) 
    cout << "move g" << (unsigned long) this << ' ' << dx << ' ' << dy << endl
      << "move g" << (unsigned long) this + 1 << ' ' << dx << ' ' << dy << endl;
}
