#!/bin/sh

eval `luarocks --lua-version 5.1 path`
gdb --args luajit -- `dirname $0`/djon.cmd.lua "$@"
