pattern=$1
find . -iname '*.c' -or -iname '*.h' | xargs egrep -iHn $pattern
