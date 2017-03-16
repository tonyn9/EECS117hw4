#!/bin/bash
#$ -N MM
#$ -q eecs117

# Runs a bunch of standard command-line
# utilities, just as an example:

echo "Script began:" `date`
echo "Node:" `hostname`
echo "Current directory: ${PWD}"

echo ""
echo "=== Running 5 trials of Matrix Multiple ==="
echo "=== with array size: 1024 1024 1024 ==="
echo "=== with block size: 512 ==="
for trial in 1 2 3 4 5 ; do
  echo "*** Trial ${trial} ***"
  ./mm 1024 1024 1024
done

echo ""
echo "=== Done! ==="

# eof
