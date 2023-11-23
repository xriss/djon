#!/bin/sh
cd `dirname $0`

eval `luarocks path`

lua ./test.lua -- "$@"

