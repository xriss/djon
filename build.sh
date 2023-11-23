#!/bin/sh
cd `dirname $0`

c/build.sh

luarocks build --local

