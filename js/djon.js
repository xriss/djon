
let core = require('bindings')('djon_core');

console.log( core.setup() )
console.log( core.clean() )
console.log( core.location() )
console.log( core.get() )
console.log( core.set() )
console.log( core.load() )
console.log( core.save() )


