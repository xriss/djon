
const fs = require('node:fs');
const wasmBuffer = fs.readFileSync('./djon_core.wasm');

let imp={}
WebAssembly.instantiate(wasmBuffer,imp).then(wasmModule => {
  console.log(imp)
  console.log("first")
  const exp = wasmModule.instance.exports;
  console.log(exp);
});


