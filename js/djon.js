
let DjonCore = require('bindings')('djon_core').DjonCore;

console.log( DjonCore )

const djoncore = new DjonCore(11)
console.log(djoncore.location())
console.log(djoncore.get())
console.log(djoncore.set({a:"b",c:"d"}))
console.log(djoncore.load())
console.log(djoncore.save())

