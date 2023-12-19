#!/bin/sh
cd `dirname $0`

echo " this will build djon_core.wasm using emscripten "

emcc \
-Wl,--no-entry \
-Wl,--export-table \
\
--optimize=1 \
\
-I../c \
-Inode_modules/node-api-headers/include \
\
-s EXPORTED_FUNCTIONS=_napi_register_wasm_v1 \
-s ERROR_ON_UNDEFINED_SYMBOLS=0 \
\
-o ./djon_core.wasm \
\
./djon_core.c \

