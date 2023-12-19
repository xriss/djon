
#include "node_api.h"

extern napi_value djon_test(napi_env env, napi_callback_info info);


#define DJON_C 1
#include "djon.h"


#define NODE_API_CALL(env, call)                                  \
	do {                                                          \
		napi_status status = (call);                              \
		if (status != napi_ok) {                                  \
			bool is_pending;                                      \
			napi_is_exception_pending((env), &is_pending);        \
			if (!is_pending) {                                    \
				napi_throw_error((env), NULL, "error");           \
			}                                                     \
			return NULL;                                          \
		}                                                         \
	} while(0)


typedef struct core_struct_functions{
	const char *name;
	napi_value (*func)(napi_env env, napi_callback_info info);
} core_struct_functions ;

static napi_value core_export_functions(napi_env env, napi_value exports, const core_struct_functions funcs[], void *data)
{
	for( const core_struct_functions *fp=funcs ; fp->name ; fp++ )
	{
		napi_value exported_function;

		NODE_API_CALL(env, napi_create_function(env,
			fp->name,
			NAPI_AUTO_LENGTH,
			fp->func,
			data,
			&exported_function));

		NODE_API_CALL(env, napi_set_named_property(env,
			exports,
			fp->name,
			exported_function));
	}
	return exports;
}

static napi_value core_create_double(napi_env env,double num)
{
	napi_value ret;
	NODE_API_CALL(env, napi_create_double(env,
		num,
		&ret));
	return ret;
}


static napi_value djon_core_locate(napi_env env, napi_callback_info info)
{
	size_t argc=8;
	napi_value argv[8];
	napi_value thisjs;
	djon_state *ds;
	NODE_API_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisjs, (void**)&ds ));

	return NULL;
}

static napi_value djon_core_get(napi_env env, napi_callback_info info)
{
	size_t argc=8;
	napi_value argv[8];
	napi_value thisjs;
	djon_state *ds;
	NODE_API_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisjs, (void**)&ds ));

	return NULL;
}

static napi_value djon_core_set(napi_env env, napi_callback_info info)
{
	size_t argc=8;
	napi_value argv[8];
	napi_value thisjs;
	djon_state *ds;
	NODE_API_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisjs, (void**)&ds ));

	return NULL;
}

static napi_value djon_core_load(napi_env env, napi_callback_info info)
{
	size_t argc=8;
	napi_value argv[8];
	napi_value thisjs;
	djon_state *ds;
	NODE_API_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisjs, (void**)&ds ));

	return NULL;
}

static napi_value djon_core_save(napi_env env, napi_callback_info info)
{
	size_t argc=8;
	napi_value argv[8];
	napi_value thisjs;
	djon_state *ds;
	NODE_API_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisjs, (void**)&ds ));

	return NULL;
}

static napi_value djon_core_new(napi_env env, napi_callback_info info)
{
	const core_struct_functions funcs[]={

		{		"locate"	,	djon_core_locate	},
		{		"get"		,	djon_core_get		},
		{		"set"		,	djon_core_set		},
		{		"load"		,	djon_core_load		},
		{		"save"		,	djon_core_save		},

		{0,0}
	};
	djon_state *ds=0;
	napi_value exports;
	
	ds=djon_setup();
	if(!ds){return NULL;}
	
	NODE_API_CALL(env, napi_create_object(env,
		&exports));
	return core_export_functions(env,exports,funcs,ds);
}


NAPI_MODULE_INIT() {
	const core_struct_functions funcs[]={

		{		"djon"	,	djon_core_new	},

		{0,0}
	};
	return core_export_functions(env,exports,funcs,0);
}

