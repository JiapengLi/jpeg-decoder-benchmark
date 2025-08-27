#!/bin/bash

CLI=./jpgdtest
IMAGES=samples/*.jpg
RESULTS=samples/results.txt
RESCSV=results.csv

uname -a > $RESULTS
for img in $IMAGES; do
    $CLI "$img" >> $RESULTS
done

echo "file,rounds,tjpgd full(us),zjpgd full(us),zjpgd central 1/4(us), zjpgd top left 1/4(us), zjpgd bottom right 1/4(us)" > $RESCSV
cat $RESULTS | grep 'file' >> $RESCSV