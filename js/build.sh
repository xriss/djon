#!/bin/sh

# copy djon.h from c dir ( npm will package it )
if [ -f ../c/djon.h ] ; then
cp ../c/djon.h .
fi

emcc -Wl,--export-table -Wl,--no-entry --optimize=3 -s WASM=1 -s STANDALONE_WASM -I./node_modules/node-addon-api -Inode_modules/node-api-headers/include -o ./djon_core.wasm ./djon_core.c

