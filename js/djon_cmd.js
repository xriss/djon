
let fs = require('fs')

let djon=require("./djon.js")

let opts={}
for( let v of (process.argv.slice(2)) )
{
	let vp=v
	if(opts.skip_opts) { vp=nil } // skip all opts

	if      (vp=="--")        {		opts.skip_opts=true		}
	else if (vp=="--djon")    {		opts.djon=true			}
	else if (vp=="--json")    {		opts.djon=false			}
	else if (vp=="--compact") {		opts.compact=true		}
	else if (vp=="--pretty")  {		opts.compact=false		}
	else if (vp=="--help")    {		opts.help=true			}
	else
	{
		if( vp.slice(0,2)=="--" ) 
		{
			console.log( "unknown option "+v )
			return(20)
		}
		else if(!opts.fname1) { opts.fname1=v }
		else if(!opts.fname2) { opts.fname2=v }
		else
		{
			console.log( "unknown option "+v )
			return(20)
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

	--djon    : output djon format
	--json    : output json format
	--compact : output compact
	--pretty  : output pretty
	--        : stop parsing options

We default to pretty output.
`)

return(0)

}


let data_input=""
let tree=null

if(opts.fname1)
{
	tree=djon.load_file(opts.fname1,"comment")
}
else
{
	let data_input=fs.readFileSync( process.stdin.fd , 'utf-8' )
	tree=djon.load(data_input,"comment")
}

let data_output=djon.save(tree,"comment",opts.djon?"djon":"",opts.compact?"compact":"")

if(opts.fname2)
{
	fs.writeFileSync(opts.fname2, data_output, 'utf8' )
}
else
{
	fs.writeFileSync( process.stdout.fd , data_output , 'utf8' )
}


