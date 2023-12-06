

local djon=require("djon")

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

if opts.help then

print([[

lua/djon.sh input.filename output.filename

	If no output.filename then write to stdout
	If no input.filename then read from stdin

Possible options are:

	--djon    : output djon format
	--json    : output json format
	--compact : output compact
	--pretty  : output pretty
	--        : stop parsing options

We default to pretty output.
]])

os.exit(0)

end


local data_input=""

if opts.fname1 then
	local fp=assert(io.open(opts.fname1,"rb"))
	data_input=fp:read("*all")
	fp:close()
else
	data_input=io.read("*all")
end

local data_tab=djon.load(data_input)

local data_output=djon.save(data_tab)

if opts.fname2 then
	local fp=assert(io.open(opts.fname2,"wb"))
	data_input=fp:write(data_output)
	fp:close()
else
	data_input=io.write(data_output)
end

os.exit(0)