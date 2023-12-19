#!/bin/sh
cd `dirname $0`

emcc \
-Wl,--no-entry \
-Wl,--export-table \
\
--optimize=1 \
\
-I../c \
-Inode_modules/node-api-headers/include \
\
-s EXPORT_KEEPALIVE \
-s ALLOW_MEMORY_GROWTH=1 \
-s STANDALONE_WASM \
-s WASM=1 \
-s EXPORTED_FUNCTIONS=_napi_register_wasm_v1 \
-s ERROR_ON_UNDEFINED_SYMBOLS=0 \
\
-o ./djon_core.wasm \
\
./djon_core.c \

