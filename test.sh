#!/bin/bash

CLI=./jpgdtest
IMAGES=samples/*.jpg
RESULTS=samples/results.txt
RESCSV=results.csv
ROUNDS=20
DELAY=0

if [ "$1" != "" ]; then
    ROUNDS=$1
fi

if [ "$2" != "" ]; then
    DELAY=$2
fi

uname -a > $RESULTS
for img in $IMAGES; do
    echo "Processing $img..."
    $CLI "$img" $ROUNDS >> $RESULTS
    if [ "$DELAY" != "0" ]; then
        sleep $DELAY
    fi
done

echo "file,rounds,tjpgd full(us),zjpgd full(us),zjpgd rect(us),zjpgd central 1/4(us),zjpgd top left 1/4(us),zjpgd bottom right 1/4(us)" > $RESCSV
cat $RESULTS | grep 'file' >> $RESCSV
