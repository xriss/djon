#!/bin/sh

em++ --no-entry -O1 -s WASM=1 -s STANDALONE_WASM -I./node_modules/node-addon-api -Inode_modules/node-api-headers/include -o ./djon_core.wasm ./djon_core.cpp

