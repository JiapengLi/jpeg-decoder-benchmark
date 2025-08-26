#!/bin/bash

CLI=./jpgdtest
IMAGES=samples/*.jpg
RESULTS=samples/results.txt
RESCSV=results.csv

uname -a > $RESULTS
for img in $IMAGES; do
    $CLI "$img" >> $RESULTS
done

echo "file,rounds,tjpgd,zjpgd" > $RESCSV
cat $RESULTS | grep 'file' >> $RESCSV