
let DjonCore = require('bindings')('djon_core').DjonCore;

console.log( DjonCore )

const example = new DjonCore(11)
console.log(example.GetValue())
// It prints 11
example.SetValue(19)
console.log(example.GetValue());

/*
console.log( core.setup() )
console.log( core.clean() )
console.log( core.location() )
console.log( core.get() )
console.log( core.set() )
console.log( core.load() )
console.log( core.save() )
*/



