
#include "node_api.h"


#define DJON_FILE (0)
#define DJON_C 1
#include "djon.h"


// we can skip providing napi_wasm_malloc as we are not using buffers

// this probably wont error, and if it does...
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


static napi_value core_get_null(napi_env env)
{
	napi_value ret;
	NODE_API_CALL(env, napi_get_null(env,
		&ret));
	return ret;
}
static napi_value core_object_set(napi_env env,napi_value obj, const char *key, napi_value val)
{
	NODE_API_CALL(env, napi_set_named_property(env,
		obj,key,val));
	return obj;
}
static napi_value core_object_get(napi_env env,napi_value obj, const char *key)
{
	napi_value ret;
	NODE_API_CALL(env, napi_get_named_property(env,
		obj,key,&ret));
	return ret;
}
static napi_value core_array_set(napi_env env,napi_value arr, int idx, napi_value val)
{
	NODE_API_CALL(env, napi_set_element(env,
		arr,idx,val));
	return arr;
}
static napi_value core_array_get(napi_env env,napi_value arr, int idx)
{
	napi_value ret;
	NODE_API_CALL(env, napi_get_element(env,
		arr,idx,&ret));
	return ret;
}

static napi_value core_create_object(napi_env env)
{
	napi_value ret;
	NODE_API_CALL(env, napi_create_object(env,
		&ret));
	return ret;
}

static napi_value core_create_array(napi_env env,int len)
{
	napi_value ret;
	NODE_API_CALL(env, napi_create_array_with_length(env,
		len,
		&ret));
	return ret;
}

static napi_value core_create_number(napi_env env,double num)
{
	napi_value ret;
	NODE_API_CALL(env, napi_create_double(env,
		num,
		&ret));
	return ret;
}

static napi_value core_create_string(napi_env env,const char *cp)
{
	napi_value ret;
	NODE_API_CALL(env, napi_create_string_utf8(env,
		cp,
		NAPI_AUTO_LENGTH,
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

	napi_value a=core_create_array(env,4);
	if(ds->error_string)
	{
		core_array_set(env,a,0,core_create_string(env,(const char *)ds->error_string));
	}
	else
	{
		core_array_set(env,a,0,core_get_null(env));
	}
	core_array_set(env,a,1,core_create_number(env,ds->error_line));
	core_array_set(env,a,2,core_create_number(env,ds->error_char));
	core_array_set(env,a,3,core_create_number(env,ds->error_idx));
	return a;
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

static void djon_core_finalizer(napi_env env, void *data, void *hint)
{
	djon_state *ds=(djon_state *)data;
//	printf("it is over\n");
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

	NODE_API_CALL(env, napi_add_finalizer(env,
		exports,ds,djon_core_finalizer,0,0));
                               
	return core_export_functions(env,exports,funcs,ds);
}


NAPI_MODULE_INIT() {
	const core_struct_functions funcs[]={

		{		"djon"	,	djon_core_new	},

		{0,0}
	};
	return core_export_functions(env,exports,funcs,0);
}

