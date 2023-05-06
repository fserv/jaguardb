#!/bin/bash

############################################################################
##
##   main driver program to test consistent-hashing scaling
##
##   The value of N tells how many records to add after each resizing
##
############################################################################


### add records after each scaling
((N=10000))


##################### 1 node ###########################
### write records to cluster
date
cfg1="ServerCfg1.txt"
echo "Insert/redistribute $N records to cluster ..."
node1=`cut -d: -f1 $cfg1`
echo ./mycli  $node1 send $N ...
./mycli  $node1 send $N
echo Done
date

sleep 5

##################### 2 nodes ###########################
### add a new node
cfg2="ServerCfg2.txt"
date
echo "adding node according to $cfg2 ..."
for node in `cut -d: -f1 $cfg2`; do
    echo "./mycli $node file $cfg2 ... "
    ./mycli $node file $cfg2 &
done

sleep 10

### redistribute data per consisten-hashing
echo "scale $node1 ..."
./mycli $node1 scale

date


### write more records to cluster now having two nodes
date
echo "Insert $N records to cluster ..."
./mycli  $node1 send $N
echo Done
date


##################### 3 nodes ###########################
sleep 10
### add a new node
cfg3="ServerCfg3.txt"
date
echo "adding node according to $cfg3 ..."
for node in `cut -d: -f1 $cfg3`; do
    ./mycli $node file $cfg3 &
done
sleep 10

### redistribute data per consisten-hashing
echo "scale nodes in $cfg2 ..."
for node in `cut -d: -f1 $cfg2`; do
    ./mycli $node scale &
done

sleep 10

date

### write more records to cluster now having 3 nodes
date
echo "Insert $N records to 3-node cluster ..."
./mycli  $node1 send $N
echo Done
date


##################### 4 nodes ###########################
sleep 10
### add a new node
cfg4="ServerCfg4.txt"
date
echo "adding node according to $cfg4 ..."
for node in `cut -d: -f1 $cfg4`; do
    ./mycli $node file $cfg4 &
done
sleep 10

### redistribute data per consisten-hashing
echo "scale nodes in $cfg3 ..."
for node in `cut -d: -f1 $cfg3`; do
    ./mycli $node scale &
done

sleep 10

date

### write more records to cluster now having 4 nodes
date
echo "Insert $N records to 4-node cluster ..."
./mycli  $node1 send $N
echo Done
date


##################### 5 nodes ###########################
sleep 10
### add a new node
cfg5="ServerCfg5.txt"
date
echo "adding node according to $cfg5 ..."
for node in `cut -d: -f1 $cfg5`; do
    ./mycli $node file $cfg5 &
done
sleep 10

### redistribute data per consisten-hashing
echo "scale nodes in $cfg4 ..."
for node in `cut -d: -f1 $cfg4`; do
    ./mycli $node scale &
done

sleep 10

date

### write more records to cluster now having 4 nodes
date
echo "Insert $N records to 5-node cluster ..."
./mycli  $node1 send $N
echo Done
date


##################### 6 nodes ###########################
sleep 10
### add a new node
cfg6="ServerCfg6.txt"
date
echo "adding node according to $cfg6 ..."
for node in `cut -d: -f1 $cfg6`; do
    ./mycli $node file $cfg6 &
done
sleep 10

### redistribute data per consisten-hashing
echo "scale nodes in $cfg5 ..."
for node in `cut -d: -f1 $cfg5`; do
    ./mycli $node scale &
done

sleep 10

date

### write more records to cluster now having 6 nodes
date
echo "Insert $N records to 6-node cluster ..."
./mycli  $node1 send $N
echo Done
date

