#!/usr/bin/wish

wm title . "Easy Calculator"
wm resizable . 0 0

entry .e -width 30 -textvariable data

foreach i1 { 1 2 3 } {
   frame [format ".bl%d" $i1]
   foreach i { 1 2 3 } {
      set n [expr $i + $i1*3 - 3]
      button [format ".bl%d.b%d" $i1 $i] -width 5 -text $n -command "append data $n"
      set z($i) [format ".bl%d.b%d" $i1 $i]
   }
   pack $z(1) $z(2) $z(3) -side left
}

pack .e .bl1 .bl2 .bl3
