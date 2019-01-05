#!/usr/bin/wish

set mem ""
set odata ""

proc = {} {
   global data odata error
   set odata $data
   if {$data == ""} return
   set data [expr 1.0*$data]
#  or with error handling
#   if [catch {set data [expr 1.0*$data]} error] {set data "";puts $error}
}

wm title . "Easy Calculator"
wm resizable . 0 0

entry .e -width 50 -textvariable data
set r 0
foreach row {
   {1 2 3 BS CE Exit}
   {4 5 6 + - (}
   {7 8 9 * / )}
   {0 . =  prev store get}
 } {
   incr r
   frame .bl$r
   set k 0
   foreach key $row {
      incr k
      switch -- $key {
         BS      {set cmd {set data [string range $data 0 end-1]}}
         CE      {set cmd {set data ""}}
         Exit    {set cmd exit}
         =       {set cmd =}
         prev    {set cmd {set data $odata}}
         store   {set cmd {set mem $data}}
         get     {set cmd {set data $mem}}
         default {set cmd "append data $key"}
      }
      pack [button .bl$r.b$k -width 5 -text $key -command "$cmd;.e icursor end"] -side left
    }
}

pack .e .bl1 .bl2 .bl3 .bl4

focus .e

bind .e <Return> =
bind . <Escape> exit

