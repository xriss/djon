#!/bin/sh

eval `luarocks path`
lua -- `dirname $0`/djon.cmd.lua "$@"
