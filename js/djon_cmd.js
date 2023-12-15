
let djon=require("djon")


local opts={}

for _,v in ipairs({...}) do

	local vp=v
	if opts.skip_opts then vp=nil end -- skip all opts

	if     vp=="--"        then		opts.skip_opts=true
	elseif vp=="--djon"    then		opts.djon=true
	elseif vp=="--json"    then		opts.djon=false
	elseif vp=="--compact" then		opts.compact=true
	elseif vp=="--pretty"  then		opts.compact=false
	elseif vp=="--help"  then		opts.help=true
	else
		if not opts.fname1 then opts.fname1=v
		elseif not opts.fname2 then opts.fname2=v
		else
			print( "unknown option "..v )
			os.exit(20)
		end
	end

end

