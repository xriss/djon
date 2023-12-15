
const fs = require('node:fs');

let DjonCore = require('bindings')('djon_core').DjonCore;

let djon=exports

djon.load_file=function(fname,...args)
{
	let it={}
	it.data = fs.readFileSync(fname, 'utf8')
	return djon.load_core(it,...args)
}

djon.load=function(data,...args)
{
	let it={}
	it.data=data
	return djon.load_core(it,...args)
}

djon.load_core=function(it,...args)
{
	it.core=new DjonCore()
	it.core.load(it.data,...args)
	djon.check_error(it)
	it.tree=it.core.get(...args)
	djon.check_error(it)
	return it.tree
}

djon.save_file=function(fname,tree,...args)
{
	let it={}
	it.tree=tree
	let data=djon.save_core(it,...args)
	fs.writeFileSync(fname, data, 'utf8')
}

djon.save=function(tree,...args)
{
	let it={}
	it.tree=tree
	return djon.save_core(it,...args)
}

djon.save_core=function(it,...args)
{
	it.core=new DjonCore()
	it.core.set(it.tree,...args)
	djon.check_error(it)
	it.data=it.core.save(...args)
	djon.check_error(it)
	return it.data
}

djon.check_error=function(it)
{
	it.err=it.core.locate()
	if(it.err[0])
	{
		throw new Error(it.err[0]+" at line "+it.err[1]+" char "+it.err[2]+" byte "+it.err[3])
	}
}
