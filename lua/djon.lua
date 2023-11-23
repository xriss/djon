

local core=require("djon.core")


--module
local M={ modname=(...) }
package.loaded[M.modname]=M
local djon=M

--[[

load a json/djon file

]]
djon.load_file=function(filename,opts)
	opts=opts or {}
	opts.filename=filename
	return djon.load(opts)
end

--[[

load a json/djon text

]]
djon.load_data=function(text,opts)
	opts=opts or {}
	opts.text=text
	return djon.load(opts)
end

--[[

load a json/djon

]]
djon.load=function(opts)

	local ds=core.setup()

end


--[[

save data in a json/djon file

]]
djon.save_file=function(filename,tab,opts)
	opts=opts or {}
	opts.filename=filename
	opts.tab=tab
	return djon.save(opts)
end

--[[

save data in a json/djon string

]]
djon.save_data=function(tab,opts)
	opts=opts or {}
	opts.tab=tab
	return djon.save(opts)
end

--[[

save data in a json/djon format

]]
djon.save=function(opts)

end

