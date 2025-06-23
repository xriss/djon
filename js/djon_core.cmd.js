#!/usr/bin/env node

import * as fs   from 'fs'

  const dat = fs.readFileSync("./djon_core.wasm")
  const bas = Buffer.from(dat).toString('base64')
  const url = `data:application/wasm;base64,${bas}`

  fs.writeFileSync("./djon_core.wasm.js",`

const data=import("${url}")
export data

`,{encoding:"utf8"})


