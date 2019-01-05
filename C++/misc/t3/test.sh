U=0
C=1000
for A in 10 1000 10000 100000 1000000;do
for ((N = 5; N < 24; N++));do
for ((S = 5; S < 25; S++)); do
let "U=$U+1"
L="-DA=$A -DC=$C -DN=$N -DS=$S"
echo $U $L
g++ -O3 $L tc2-test.cpp && ./a.out >zt
g++ -O3 $L tc2.cpp && ./a.out >z
diff -urN z zt || exit
done
done
done

