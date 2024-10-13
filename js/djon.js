

import fs from 'fs';
import { Environment, napi } from 'napi-wasm';

const wdat = fs.readFileSync( new URL('./djon_core.wasm', import.meta.url) )

const mod = await WebAssembly.instantiate( wdat , {napi:napi } )
let djoncore_env = new Environment(mod.instance);
let djoncore=djoncore_env.exports.djoncore

let djon={}
export default djon

/*

merge comments in com into the data in dat. Can also just be used to 
convert dat into a comments format if com is null.

*/
djon.merge_comments=function( dat , com )
{
	let out=dat
	if( Array.isArray(dat) || Array.isArray(com) ) // output must be array
	{
		out=[dat] // must be array
		if( Array.isArray(com) ) // copy comments
		{
			for( let i=1 ; i<com.length ; i++ ){ out[i]=com[i] }
			com=com[1]
		}
	}
	if( typeof(dat)=="object" ) // need to recurse
	{
		let o={}
		for( let n in dat )
		{
			let d=dat[n]
			let c = (typeof(com)=="object")&&(com) ? com[n] : null
			o[n]=djon.merge_comments(d,c)
		}
		if( Array.isArray(out) ){ out[1]=o }else{ out=o }
	}
	return out
}

/*

Remove comments converting com back into standard json data and 
returning it.

*/
djon.remove_comments=function( com )
{
	let out=com
	if( Array.isArray(out) ) // strip comments
	{
		out=out[0]
	}
	if( typeof(out)=="object" ) // need to copy and recurse
	{
		let o={}
		for( let n in out ){ o[n]=djon.remove_comments(out[n]) }
		out=o
	}
	return out
}

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
	it.core=new djoncore()
	it.core.load(it.data,...args)
	djon.check_error(it)
	it.tree=it.core.get(...args)
	djon.check_error(it)
	return it.tree
}

djon.save_comments=function(fname,tree,...args)
{
	let com ; try{ // ignore errors (probably missing file)
		com=djon.load_file(fname,"comments",...args)
	}catch(e){}
	
	let it={}
	it.tree=djon.merge_comments( tree , com ) // merge comments
	let data=djon.save_core(it,"comments","djon",...args)
	fs.writeFileSync(fname, data, 'utf8')
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
	it.core=new djoncore()
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
