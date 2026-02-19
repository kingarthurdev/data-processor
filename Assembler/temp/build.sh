#!/bin/sh

cd ../../../../../../../../
cd autograder/source || exit 1

echo "Listing files in autograder/source directory:"
ls -a

#find .

files=(
./hw3-int-suite/tests/generated_inputs/invalid/labels/invalid2.tk
)

for f in "${files[@]}"; do
  echo "===== $f ====="
  cat "$f"
  echo
done
