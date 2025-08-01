
package = "djon"

version = "1.241016-20"

source = {
	url = "git://github.com/xriss/djon",
	tag = "v1.241016.20"
}

description = {
	homepage = "https://github.com/xriss/djon",
	license = "MIT",
}

dependencies = {
}

external_dependencies = {
}

build = {
	type = "builtin",
	modules = {
		["djon.core"] = {
			sources = {
				"lua/djon.core.c",
			},
			incdirs = {
				"c",
				"lua",
			},
		},
		["djon"] = "lua/djon.lua",
	},
}
