
local djon=require("djon")

print("roundtrip data example")


-- normal json data structure
local data_tab=djon.load_file("inp.djon")


-- modift data





-- load comments
local data_com=djon.load_file("inp.djon","comments")

-- merge changeddata back into comments

-- save with comments
djon.save_file("out.djon",data_com,"djon","comments")
