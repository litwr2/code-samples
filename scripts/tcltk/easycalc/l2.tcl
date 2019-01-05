#!/usr/bin/wish

wm title . "Easy Calculator"
wm resizable . 0 0

set mem ""
set odata ""

proc = {} {
   global data odata
   set odata $data
   if {$data == ""} return
   set data [expr 1.0*$data]  ;#результат - вещественный
   .e icursor end
}

proc ins {d} {
   global data
   append data $d
   .e icursor end
}

entry .e -width 50 -textvariable data
frame .bl1
button .bl1.b1 -width 5 -text 1 -command {ins 1}
button .bl1.b2 -width 5 -text 2 -command {ins 2}
button .bl1.b3 -width 5 -text 3 -command {ins 3}
button .bl1.b4 -width 5 -text BS -command {set data [string range $data 0 end-1]}
button .bl1.b5 -width 5 -text CE -command {set data ""}
button .bl1.b6 -width 5 -text Exit -command exit
pack .bl1.b1 .bl1.b2 .bl1.b3 .bl1.b4 .bl1.b5 .bl1.b6 -side left

frame .bl2
button .bl2.b1 -width 5 -text 4 -command {ins 4}
button .bl2.b2 -width 5 -text 5 -command {ins 5}
button .bl2.b3 -width 5 -text 6 -command {ins 6}
button .bl2.b4 -width 5 -text + -command {ins +}
button .bl2.b5 -width 5 -text - -command {ins -}
button .bl2.b6 -width 5 -text ( -command {ins (}
pack .bl2.b1 .bl2.b2 .bl2.b3 .bl2.b4 .bl2.b5 .bl2.b6 -side left

frame .bl3
button .bl3.b1 -width 5 -text 7 -command {ins 7}
button .bl3.b2 -width 5 -text 8 -command {ins 8}
button .bl3.b3 -width 5 -text 9 -command {ins 9}
button .bl3.b4 -width 5 -text * -command {ins *}
button .bl3.b5 -width 5 -text / -command {ins /}
button .bl3.b6 -width 5 -text ) -command {ins )}
pack .bl3.b1 .bl3.b2 .bl3.b3 .bl3.b4 .bl3.b5 .bl3.b6 -side left

frame .bl4
button .bl4.b1 -width 5 -text 0 -command {ins 0}
button .bl4.b2 -width 5 -text . -command {ins .}
button .bl4.b3 -width 5 -text = -command =
button .bl4.b4 -width 5 -text prev -command {set data $odata; .e icursor end}
button .bl4.b5 -width 5 -text store -command {set mem $data}
button .bl4.b6 -width 5 -text get -command {set data $mem; .e icursor end}
pack .bl4.b1 .bl4.b2 .bl4.b3 .bl4.b4 .bl4.b5 .bl4.b6 -side left

pack .e .bl1 .bl2 .bl3 .bl4

focus .e

bind .e <Return> =
bind . <Escape> exit
