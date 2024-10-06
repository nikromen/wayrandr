#!/bin/bash


set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
for i in $(find $1/xml -name '*.ui'); do
    echo "Processing $i"
    base=$(basename $i)
    dir=$(dirname $i)
    parent_dir=$(dirname $dir)
    destination="$parent_dir/generated_ui/${base%.ui}.py"
    echo "rm -f $destination"
    rm -f $destination
    echo "pyside6-uic $i -o $destination"
    pyside6-uic $i -o $destination
done
