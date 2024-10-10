#!/bin/sh

# copy test file
cp `dirname $0`/base.djon `dirname $0`/test.djon

eval `luarocks --lua-version 5.1 path`
luajit -- `dirname $0`/eg.lua "$@"
