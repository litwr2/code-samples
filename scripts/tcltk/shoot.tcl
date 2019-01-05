#!/usr/bin/wish
package require Tclx


canvas .c -width 320 -height 200 -background darkgreen

set s 1
set x -20
set y 60
set count 1
set hits 0
set speed 1
set scoreline {Speed = $speed    Targets = $count    Hits = $hits  \
  Ratio = [expr $hits*100/$count]%}

proc moves {} {
  global s x y count hits scoreline speed
  set s 1
  set yo $y
  while {$s} {
    .c move target 1 0
    set x [expr $x + 1]
    if {$x >= 340} {
      incr count
      set yo [expr [random 161] + 20]
      .c move target [expr -$x - 20] [expr $yo - $y]
      set y $yo
      set x -20
      .c itemconfigure score -text [eval concat $scoreline]
    }
    set xs 20
    for {set i 0} {$i < $speed} {incr i} {set xs [expr $xs*[random 2]]}
    after $xs
    update
  }
  while {$y < 220} {
    .c move target 0 1
    incr y
#    after 1
    update
  }
  incr hits
  incr count
  .c itemconfigure score -text [eval concat $scoreline]
  .c itemconfigure target -fill red
  set yo [expr [random 161] + 20]
  .c move target [expr -$x - 20] [expr $yo - $y]
  set x -20
  set y $yo
  moves
}

pack .c -expand 1 -side left

.c create oval -40 40 0 80 -fill red -outline black -tag target
.c create text 160 190 -fill magenta -tag score -text [eval concat $scoreline]

.c bind target <Button-1> {%W itemconfigure current -fill black; set s 0}

button .b -text Exit -width 10 -command exit
scale .s -from 1 -to 5 -digits 1 -label Speed -tickinterval 1 \
  -orient h -showvalue no \
  -command {set hits 0; set count 0; set speed}

pack .s .b

moves
