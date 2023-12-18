

import fs from 'fs';

import { Environment, napi } from 'napi-wasm';

let mod = await WebAssembly.instantiate( fs.readFileSync('./djon_core.wasm') , {
  napi: napi
});



// Create an environment.
let env = new Environment(mod.instance);
let exports = env.exports;

console.log("napi")
console.log(exports)

console.log(exports.test())

