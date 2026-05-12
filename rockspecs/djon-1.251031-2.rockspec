
package = "djon"

version = "1.251031-2"

source = {
	url = "git://github.com/xriss/djon",
	tag = "v1.251031.2"
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
