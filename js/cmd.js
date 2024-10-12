
import util from "util";
import { promises as pfs } from 'fs';

import djon from "./djon.js";

let opts={}
for( let v of (process.argv.slice(2)) )
{
	let vp=v
	if(opts.skip_opts) { vp=null } // skip all opts

	if      (vp=="--")         {		opts.skip_opts=true		}
	else if (vp=="--djon")     {		opts.djon=true			}
	else if (vp=="--json")     {		opts.djon=false			}
	else if (vp=="--compact")  {		opts.compact=true		}
	else if (vp=="--strict")   {		opts.strict=true		}
	else if (vp=="--comments") {		opts.comments=true		}
	else if (vp=="--help")     {		opts.help=true			}
	else
	{
		if( vp && vp.slice(0,2)=="--" ) 
		{
			console.log( "unknown option "+v )
			process.exit(20)
		}
		else if(!opts.fname1) { opts.fname1=v }
		else if(!opts.fname2) { opts.fname2=v }
		else
		{
			console.log( "unknown option "+v )
			process.exit(20)
		}
	}
}
//console.log(opts)

if(opts.help)
{
console.log(`

js/djon.sh input.filename output.filename

	If no output.filename then write to stdout
	If no input.filename then read from stdin

Possible options are:

	--djon     : output djon format
	--json     : output json format
	--compact  : compact output format
	--strict   : require input to be valid json ( bitch mode )
	--comments : comments to djon or djon to comments
	--         : stop parsing options

The comments flag adjusts input or output depending on if the output is 
djon ( which can contain comments ) or json ( which can not ) The json 
side has is structure modified to contain comments.

`)

process.exit(0)

}

let data_input 
if(opts.fname1)
{
	data_input = await pfs.readFile( opts.fname1 , 'utf-8' )
}
else
{
	let readstdin = async function()
	{
		const chunks = [];
		for await (const chunk of process.stdin) { chunks.push(chunk); }
		return Buffer.concat(chunks).toString('utf8');
	}
	data_input=await readstdin()
}

let tree=djon.load(data_input,opts.strict?"strict":"",(opts.comments&&(!opts.djon))?"comments":"")

let data_output=djon.save(tree,opts.djon?"djon":"",opts.compact?"compact":"",opts.strict?"strict":"",(opts.comments&&(opts.djon))?"comments":"")

if( opts.fname2 )
{
	await pfs.writeFile( opts.fname2 , data_output , 'utf8' )
}
else
{
	await process.stdout.write( data_output , 'utf8' )
}

//process.exit(0)
