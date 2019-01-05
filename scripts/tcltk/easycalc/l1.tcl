#!/usr/bin/wish

wm title . "Easy Calculator"  ;#
wm resizable . 0 0            ;#нельзя менять размер окна

entry .e -width 30 -textvariable data  ;#поле ввода
frame .bl1                             ;#собираем клавиши по три
button .bl1.b1 -width 5 -text 1 -command {append data 1}
button .bl1.b2 -width 5 -text 2 -command {append data 2}
button .bl1.b3 -width 5 -text 3 -command {append data 3}
pack .bl1.b1 .bl1.b2 .bl1.b3 -side left ;#пакуем клавиши

frame .bl2
button .bl2.b1 -width 5 -text 4 -command {append data 4}
button .bl2.b2 -width 5 -text 5 -command {append data 5}
button .bl2.b3 -width 5 -text 6 -command {append data 6}
pack .bl2.b1 .bl2.b2 .bl2.b3 -side left

frame .bl3
button .bl3.b1 -width 5 -text 7 -command {append data 7}
button .bl3.b2 -width 5 -text 8 -command {append data 8}
button .bl3.b3 -width 5 -text 9 -command {append data 9}
pack .bl3.b1 .bl3.b2 .bl3.b3 -side left

pack .e .bl1 .bl2 .bl3

