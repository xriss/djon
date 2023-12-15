#!/bin/sh
cd `dirname $0`

c/build.sh

cp c/djon.h lua/djon.h
luarocks --lua-version 5.1 build --local

cp c/djon.h js/djon.h
npm install --prefix js

