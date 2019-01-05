#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cmath>
#include <set>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>
using namespace std;

struct FD {
   int id;
   double min, max;
};

struct FDless {
    bool operator() (const FD &fd1, const FD &fd2) {
        return fd1.id < fd2.id;
    }
};

set<FD, FDless> functions;
Fl_Box *box;
int xmin, xmax, ymin, ymax;
int colortab[] = {FL_RED, FL_CYAN, FL_YELLOW, FL_WHITE, FL_MAGENTA};

double sinpluscos(double x) {
   return cos(x) + sin(x);
}

double oneplussqr(double x) {
   return 1 + x*x;
}

double sec(double x) {
   return 1/cos(x);
}

double cosec(double x) {
   return 1/sin(x);
}

double sqrt1(double x) {
   return sqrt(9 - x*x);
}

double invx(double x) {
   return 1/x;
}

double const5(double) {
   return 5;
}

struct FNdata {
   const char* fn;
   double (*fd)(double);
} fndata[] = {{" ", 0}, {"sin(x)", sin}, {"cos(x)", cos}, {"tg(x)", tan},
   {"sh(x)", sinh}, {"ch(x)", cosh}, {"th(x)", tanh}, {"ln(x)", log}, 
   {"sin(x)+cos(x)", sinpluscos}, {"arctg(x)", atan}, {"1+x²", oneplussqr},
   {"sec(x)", sec}, {"cosec(x)", cosec}, {"e^x", exp}, {"√(9-x²)", sqrt1},
   {"1∕x", invx}, {"const=5", const5}, {0, 0}};

int get_rnd_screen_x() {
   return (double)rand()*Fl::w()/RAND_MAX;
}

int get_rnd_screen_y() {
   return (double)rand()*Fl::h()/RAND_MAX;
}

string int2str(int i) {
   ostringstream os;
   os << i;
   return os.str();
}

void button1_callback(Fl_Widget*) {
   Fl_Window *window = new Fl_Window(340, 180);
   Fl_Box *box = new Fl_Box(20, 40, 300, 100, "Hello, World!");
   box->box(FL_UP_BOX);
   //box->labelfont(FL_BOLD + FL_ITALIC);
   box->labelsize(36);
   //box->labeltype(FL_SHADOW_LABEL);
   window->position(get_rnd_screen_x(), get_rnd_screen_y());
   window->end();
   window->show();
}

void button2_callback(Fl_Widget*) {
   exit(0);
}

void show_fn_list() {
   ostringstream os;
   os <<  "Entered functions:\n";
   for (set<FD, FDless>::iterator i = functions.begin(); i != functions.end(); ++i)
      os << fndata[i->id].fn << '[' << i->min << ',' << i->max << "] "; 
   box->copy_label(os.str().c_str());
}

void button3_callback(Fl_Widget*) {
   functions.clear();
   show_fn_list();
}

void choice1_callback(Fl_Widget *w) {
   if (functions.size() < 4) {
      FD fd;
      if ((fd.id = ((Fl_Choice*)w)->value()) == 0)
         return;
      do {
         const char *input;
         do
            input = fl_input("Input min x:", "");
         while (input == 0 || *input == 0);
         fd.min = atof(input);
         do
            input = fl_input("Input max x:", "");
         while (input == 0 || *input == 0);
         fd.max = atof(input);
      } while (fd.min + .2 > fd.max);
      for (set<FD, FDless>::iterator i = functions.begin(); i != functions.end(); ++i)
          if (i->id == fd.id) {
             functions.erase(*i);
             break;
          }
      functions.insert(fd);
      show_fn_list();
      ((Fl_Choice*)w)->value(0);
   }
}

void choice2_callback(Fl_Widget* w) {
   xmin = ((Fl_Choice*)w)->value() - 11;
}

void choice3_callback(Fl_Widget* w) {
   xmax = ((Fl_Choice*)w)->value() + 2;
}

void choice4_callback(Fl_Widget* w) {
   ymin = ((Fl_Choice*)w)->value() - 11;
}

void choice5_callback(Fl_Widget* w) {
   ymax = ((Fl_Choice*)w)->value() + 2;
}

Fl_Window *gwindow = 0;

class Drawing : public Fl_Widget {
   void draw() {
      fl_color(FL_DARK3);
      fl_rectf(x(),y(),w(),h());
      fl_color(FL_GREEN);
      int h1 = (h() - 11)*ymax/(ymax - ymin) + 7;
      int w1 = (w() - 11)*xmin/(xmin - xmax) + 4;
      fl_line(w1, 0, w1, h()); //y-axis
      fl_line(w1 - 5, 5, w1, 0, w1 + 5, 5);
      double ystep = (double)(h() - 11)/(ymax - ymin), yi = h() - 4;
      for (int i = ymin; i <= ymax; i++) {
         if (i) {
            fl_line(w1 - 2, yi, w1 + 2, yi);
            fl_draw(int2str(i).c_str(), w1 + 8, yi + 4);
         }
         yi -= ystep;
      }
      fl_line(0, h1, w(), h1); //x-axis
      fl_line(w() - 5, h1 - 5, w(), h1, w() - 5, h1 + 5);
      double xstep = (double)(w() - 11)/(xmax - xmin), xi = 4;
      for (int i = xmin; i <= xmax; i++) {
         if (i) {
            fl_line(xi, h1 - 2, xi, h1 + 2);
            fl_draw(int2str(i).c_str(), xi - 5, h1 + 14);
         }
         xi += xstep;
      }
      fl_draw("0", w1 - 10, h1 - 4);
      fl_rect(w1 - 1, h1 - 1, 3, 3);
      fl_draw("x", w() - 14, h1 - 8);
      fl_draw("y", w1 - 16, 14);
      int color = 0;
      for (set<FD, FDless>::iterator i = functions.begin(); i != functions.end(); ++i) {
         double fxmin = i->min, fxmax = i->max, xmin1 = xmin - 4/xstep,
            xmax1 = xmax + 7/xstep, ymax1 = ymax + 7/ystep, ymin1 = ymin - 4/ystep;
         if (fxmin < xmin1) fxmin = xmin1;
         if (fxmax > xmax1) fxmax = xmax1;
         int xfcur = 0;
         if (xmin1 <= fxmin)
            xfcur = (w() - 11)*(fxmin - xmin)/(xmax - xmin) + 4;
         double yc, oyc = 0;
         fl_color(colortab[color++]);
         fl_begin_line();
         for (double xc = fxmin; xc < fxmax + 1/xstep/2; xc += 1/xstep) {
            if (xc > fxmax) xc = fxmax;
            yc = (*fndata[i->id].fd)(xc);
            if (yc < ymin1) yc = ymin1 - 1;
            if (yc > ymax1) yc = ymax1 + 1;
            if (yc < ymin1 && oyc > ymax1 || yc > ymax1 && oyc < ymin1
                                                           || isnan(yc) || isnan(oyc)) {
//isnan(x) - is Not A Number - true when x has undefined value
               fl_end_line();
               fl_begin_line();
            }
            fl_vertex(xfcur++, (h() - 11)*(ymax - yc)/(ymax - ymin) + 7);
            oyc = yc;
         }
         fl_end_line();
         fl_draw(fndata[i->id].fn, 16, color*14);
      }
    }
public:
    Drawing(int X, int Y, int W, int H) : Fl_Widget(X, Y, W, H) {}
    ~Drawing() {gwindow = 0;}
};

void window_callback(Fl_Widget* w) {
   Fl::delete_widget(w);
}

void button4_callback(Fl_Widget*) {
   if (functions.size() && xmax && xmin && ymax && ymin) 
      if (gwindow == 0) {
         gwindow = new Fl_Window(500, 500, "Graphics");
         gwindow->resizable(*new Drawing(0, 0, 500, 500));
         gwindow->position(get_rnd_screen_x(), get_rnd_screen_y());
         gwindow->end();
         gwindow->show();
         gwindow->callback(window_callback);
      }
      else
         gwindow->redraw();
}

int main(int argc, char **argv) {
   Fl_Window window(340, 280, "Task3");
   Fl_Button button1(10, 10, 320, 20, "Hello");
   button1.labelsize(12);
   button1.callback(button1_callback);
   Fl_Button button2(10, 40, 320, 20, "Exit");
   button2.labelsize(12);
   button2.callback(button2_callback);
   Fl_Choice choice1(140, 70, 180, 20, "Select function:");
   for (int i = 0; fndata[i].fn; ++i)
      choice1.add(fndata[i].fn);
   choice1.callback(choice1_callback);
   Fl_Button button3(10, 100, 320, 20, "Clear functions set");
   button3.labelsize(12);
   button3.callback(button3_callback);
   box = new Fl_Box(10, 130, 320, 40);
   box->box(FL_NO_BOX);
   box->labelsize(12);
   show_fn_list();
   Fl_Choice choice2(70, 180, 50, 20, "Xmin:");
   for (int i = -11; i < -1; ++i)
      choice2.add(int2str(i).c_str());
   choice2.callback(choice2_callback);
   Fl_Choice choice3(250, 180, 50, 20, "Xmax:");
   for (int i = 2; i < 12; ++i)
      choice3.add(int2str(i).c_str());
   choice3.callback(choice3_callback);
   Fl_Choice choice4(70, 210, 50, 20, "Ymin:");
   for (int i = -11; i < -1; ++i)
      choice4.add(int2str(i).c_str());
   choice4.callback(choice4_callback);
   Fl_Choice choice5(250, 210, 50, 20, "Ymax:");
   for (int i = 2; i < 12; ++i)
      choice5.add(int2str(i).c_str());
   choice5.callback(choice5_callback);
   Fl_Button button4(10, 240, 320, 20, "Draw function graphs");
   button4.labelsize(12);
   button4.callback(button4_callback);
   //window->position(get_rnd_screen_x(), get_rnd_screen_y());
   window.end();
   window.show(argc, argv);
   return Fl::run();
}

