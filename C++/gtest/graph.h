#ifndef _GRAPH
#define _GRAPH

#include <map>
#include <string>
using namespace std;

#define XSIZE 640
#define YSIZE 400
#define BG 9

extern char colors [][12];
extern map <string, int> colorsmap;
void inimap ();

class Point {
protected:
  double x, y;
  int color, vis;
  static int q;
public:
  int xsign, ysign;
  Point (double (*) (double, int), double (*) (double, int), int, int, const char*);
  virtual void toggle ();
  virtual void changeColor (int color0) {color = color0;}
  virtual void move (double, double);
  double getx () {return x;}
  double gety () {return y;}
  int getColor () {return color;}
  int getVis () {return vis;}
  static int getQ () {return q;}
  double (*xt) (double, int);
  double (*yt) (double, int);
};

class Circle : public Point {
  int radius;
public:
  Circle (double (*) (double, int), double (*) (double, int), int, int, int, const char*);
  void toggle ();
  int getRadius () {return radius;}
  void changeColor (int);
};

class Rectangle : public Point {
  int xb, yb;
public:
  Rectangle (double (*) (double, int), double (*) (double, int), int,
                 int, int, int, const char*);
  void toggle ();
  int getxb () {return xb;}
  int getyb () {return yb;}
  void move (double, double);
  void changeColor (int);
};

class Ring : public Circle {
  int ir;
public:
  Ring (double (*) (double, int), double (*) (double, int), int, int, int, int, const char*);
  void toggle ();
  int getiRadius () {return ir;}
  void changeColor (int);
  void move (double, double);
};

class Letter : public Point {
protected:
  char ch;
public:
  Letter (double (*) (double, int), double (*) (double, int), char, int, 
       int, const char*);
  void toggle ();
  char getChar () {return ch;}
  void changeColor (int);
};

class CircLett : public Circle, public Letter {
public:
  CircLett (double (*) (double, int), double (*) (double, int), int,
                 int, int, const char*, char, const char*);
  void toggle ();
  void changeColor (int, int);
  void move (double, double); 
};

#endif
