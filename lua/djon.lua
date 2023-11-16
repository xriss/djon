

local core=require("djon.core")


--module
local M={ modname=(...) }
package.loaded[M.modname]=M
local djon=M

djon.load=function(fname,opts)

end

djon.save=function(fname,opts)

end
