#!/bin/bash

echo "Make sure /home/jaguar/jaguar/lib has .so and jaguarnode.node files"

echo "node-gyp configure clean ..."
node-gyp configure clean

echo "node-gyp configure build ..."
node-gyp configure build

echo "node-gyp configure build done"

if [[ -f "build/Release/jaguarnode.node" ]]; then
	/bin/cp -f build/Release/jaguarnode.node .
else
    echo "build/Release/jaguarnode.node not found"
fi
