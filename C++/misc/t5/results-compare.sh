TESTFILE=test.fme
rm -rf results-fme results-linux test-fme.linux test-fme-dir
mkdir test-fme-dir
cd test-fme-dir
sed 's/md/mkdir/;s/mf/touch/;s! /! ./!g;s/cp/cp -R/;s/rm/rm -rf/' ../$TESTFILE > ../test-fme.linux
. ../test-fme.linux
tree --charset ascii --noreport -nF | sed 's/--/_/;s/`/|/' | sed 's!^.$!/!' >../results-linux
cd ..
./test-fme <$TESTFILE >results-fme
diff -w results-linux results-fme && (echo ok; rm -rf results-fme results-linux test-fme.linux test-fme-dir)

