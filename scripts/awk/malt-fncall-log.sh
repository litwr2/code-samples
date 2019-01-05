T="Use a correct core number as a parameter, e.g., 0x0101 or 0x0\nIt's possible to use the second parameter - a cpu tracelog file"
case $# in
[12]) true;;
*) echo -e $T; exit;;
esac
FN=${1,,}
if [ .$1 = . ]; then echo -e $T; exit; fi
if [ $((0x`printf "%04x" $1` != $1)) = 1 -o ${FN:0:2} != 0x ]; then echo -e $T; exit; fi
if [ $# = 1 ]; then MALT_TRACE_CPU=$FN make emu; fi
FN=maltemu.`printf "%04x" $FN`.log
if ! [ -f $FN ]; then echo -e $T; exit; fi
FE=`echo *.elf`
echo Please wait... Tracing is generating...
if [ $FE -nt $FE.lst ]; then microblaze-xilinx-elf-objdump -DSCz $FE >$FE.lst; fi
(echo 'BEGIN{'
grep -P '[0-9a-f]{8} <.*>:' $FE.lst|sort|awk '{print "adata[++n]="strtonum("0x"$1)";tdata[n]=\""$2$3$4$5$6$7$8$9$10$11$12$13$14$15$16"\""}'
echo '}
{addr=strtonum("0x"$3);
for(i = 1; i <= n; i++)if(adata[i]>=addr)break
if(adata[i]>addr)i--
printf "%2d %s %x %x %s\n", $1, $2, addr, adata[i], tdata[i]
}') >getfn-search.awk
awk '/rtsd/ {
   p = index($0, "PC=")
   s = substr($0, p + 3)
   p = index(s, ";")
   s = substr(s, 1, p - 1)
   printf "%d rtsd %s\n", level--, s
}
/Dealing with jump to/ {
   if (brald) {
       brald = 0
       printf "%d brald %s\n", ++level, $5
   }
   else if (brld) {
       brld = 0
       printf "%d brld %s\n", ++level, $5
   }
}
/brald/ {
   brald = 1
}
/bralid/ {
   p = index($0, "// ")
   s = substr($0, p + 3)
   printf "%d bralid %s\n", ++level, 0
}
/brld/ {
   brld = 1
}
/brlid/ {
   p = index($0, "// ")
   s = substr($0, p + 3)
   printf "%d brlid %s\n", ++level, s
}' $FN | gawk -f getfn-search.awk >${FN%%log}calls.log
rm getfn-search.awk
