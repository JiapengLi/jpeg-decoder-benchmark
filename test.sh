#!/bin/bash

CLI=./jpgdtest
IMAGES=samples/*.jpg
RESULTS=samples/results.txt

uname -a > $RESULTS
for img in $IMAGES; do
    $CLI "$img" >> $RESULTS
done
