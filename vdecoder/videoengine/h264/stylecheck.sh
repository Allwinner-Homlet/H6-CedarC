#!/bin/bash

#-------------------------------------------------------------------------------
# usage:
#       ./stylecheck.sh path filename
# It will check all C/C++ source code or head code in the "path".
# The result will be save to "filename".
#-------------------------------------------------------------------------------

# default path and filename
DOPATH=.
RESULT=result.txt

#
if [ $# -eq 1 ]; then
    DOPATH=$1
elif [ $# -eq 2 ]; then
    DOPATH=$1
    RESULT=$2
fi

echo "#-------------------------------------------------------------------------"
echo "#     find in path: $DOPATH"
echo "#     style check results will save to file: $RESULT"
echo "#-------------------------------------------------------------------------"

# if there has be a file named $RESULT, delete it first.
if [ -e $RESULT ]; then
    rm $RESULT
fi

find $DOPATH -name '*.cpp' | xargs.exe -n1 vera++.exe --profile android >> $RESULT
find $DOPATH -name '*.c' | xargs.exe -n1 vera++.exe --profile android >> $RESULT
find $DOPATH -name '*.h' | xargs.exe -n1 vera++.exe --profile android >> $RESULT
