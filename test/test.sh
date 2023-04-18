#!/bin/sh

mkdir -p output
ulimit -c unlimited

date
echo "Test unit.sql ..."
./jagdb -v < unit.sql > output/unit.out
date


echo "Test time.sql ..."
./jagdb -v < time.sql > output/time.out
date

echo "Test geo.sql ..."
./jagdb -v < geo.sql > output/geo.out
date

