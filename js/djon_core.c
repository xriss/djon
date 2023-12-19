
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


static napi_value core_null(napi_env env)
{
	napi_value ret;
	NODE_API_CALL(env, napi_get_null(env,
		&ret));
	return ret;
}

static napi_value core_boolean(napi_env env,int num)
{
	napi_value ret;
	NODE_API_CALL(env, napi_get_boolean(env,
		num,
		&ret));
	return ret;
}

static napi_value core_object(napi_env env)
{
	napi_value ret;
	NODE_API_CALL(env, napi_create_object(env,
		&ret));
	return ret;
}

static napi_value core_array(napi_env env,int len)
{
	napi_value ret;
	NODE_API_CALL(env, napi_create_array_with_length(env,
		len,
		&ret));
	return ret;
}

static napi_value core_number(napi_env env,double num)
{
	napi_value ret;
	NODE_API_CALL(env, napi_create_double(env,
		num,
		&ret));
	return ret;
}

// alloced, so you will need to free this
static const char * core_get_string(napi_env env,napi_value val)
{
	char *cp;
	size_t size=0;
	size_t len=0;

	NODE_API_CALL(env, napi_get_value_string_utf8(env,
		val,0,0,&size));
		
	size=size+1; // term
	cp=malloc(size);
	if(!cp){return 0;}

	NODE_API_CALL(env, napi_get_value_string_utf8(env,
		val,cp,size,&len));

	return cp;
}

static napi_value core_val_set(napi_env env,napi_value obj, napi_value key, napi_value val)
{
	NODE_API_CALL(env, napi_set_property(env,
		obj,key,val));
	return obj;
}
static napi_value core_str_set(napi_env env,napi_value obj, const char *key, napi_value val)
{
	NODE_API_CALL(env, napi_set_named_property(env,
		obj,key,val));
	return obj;
}
static napi_value core_str_get(napi_env env,napi_value obj, const char *key)
{
	napi_value ret;
	NODE_API_CALL(env, napi_get_named_property(env,
		obj,key,&ret));
	return ret;
}
static napi_value core_num_set(napi_env env,napi_value arr, int idx, napi_value val)
{
	NODE_API_CALL(env, napi_set_element(env,
		arr,idx,val));
	return arr;
}
static napi_value core_num_get(napi_env env,napi_value arr, int idx)
{
	napi_value ret;
	NODE_API_CALL(env, napi_get_element(env,
		arr,idx,&ret));
	return ret;
}

static napi_value core_string(napi_env env,const char *cp)
{
	napi_value ret;
	NODE_API_CALL(env, napi_create_string_utf8(env,
		cp,
		strlen(cp),
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

	napi_value a=core_array(env,4);
	if(ds->error_string)
	{
		core_num_set(env,a,0,core_string(env,(const char *)ds->error_string));
	}
	else
	{
		core_num_set(env,a,0,core_null(env));
	}
	core_num_set(env,a,1,core_number(env,ds->error_line));
	core_num_set(env,a,2,core_number(env,ds->error_char));
	core_num_set(env,a,3,core_number(env,ds->error_idx));
	return a;
}

static napi_value djon_core_get_value(napi_env env,djon_state *ds,int idx)
{
djon_value *v=djon_get(ds,idx);
int ai,len;
napi_value arr;
napi_value obj;
	if(!v) { return core_null(env); }
	switch(v->typ&DJON_TYPEMASK)
	{
		case DJON_ARRAY:
			len=0;
			for( int vi=v->val ; vi ; vi=djon_get(ds,vi)->nxt )
			{
				len++;
			}
			arr=core_array(env,len);
			ai=0;
			for( int vi=v->val ; vi ; vi=djon_get(ds,vi)->nxt )
			{
				core_num_set( env , arr , vi , djon_core_get_value( env , ds ,vi ) );
				ai++;
			}
			return arr;
		break;
		case DJON_OBJECT:
			obj=core_object(env);
			for( int ki=v->key ; ki ; ki=djon_get(ds,ki)->nxt )
			{
				core_val_set( env , obj , djon_core_get_value( env , ds , ki ) , djon_core_get_value( env , ds , djon_get(ds,ki)->val ) );
			}
			return obj;
		break;
		case DJON_STRING:
			return core_string( env , v->str );
		break;
		case DJON_NUMBER:
			return core_number( env , v->num );
		break;
		case DJON_BOOL:
			return core_boolean( env , v->num ? true : false );
		break;
	}
	
	return core_null(env);
}
static napi_value djon_core_get(napi_env env, napi_callback_info info)
{
	size_t argc=8;
	napi_value argv[8];
	napi_value thisjs;
	djon_state *ds;
	NODE_API_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisjs, (void**)&ds ));

	ds->comment=0;
	for(int i=1;(size_t)i<argc;i++) // check string flags in args
	{
		const char *cp=core_get_string(env,argv[i]);
		if( strcmp(cp,"comment")==0 ) { ds->comment=1; }
		free((void*)cp);
	}

	return djon_core_get_value(env,ds,ds->parse_value);
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

	// this will get auto free on object cleanup
	ds->data = (char*) core_get_string(env,argv[0]);
	if(!ds->data){return NULL;}
	ds->data_len=strlen(ds->data);

	ds->strict=0;
	for(int i=1;(size_t)i<argc;i++) // check string flags in args
	{
		const char *cp=core_get_string(env,argv[i]);
		if( strcmp(cp,"strict")==0 ) { ds->strict=1; }
		free((void*)cp);
	}

	djon_parse(ds);

	return NULL;
}

static napi_value djon_core_save(napi_env env, napi_callback_info info)
{
	size_t argc=8;
	napi_value argv[8];
	napi_value thisjs;
	djon_state *ds;
	NODE_API_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisjs, (void**)&ds ));

	napi_value ret;

	int write_djon=0;

	ds->write=&djon_write_data; // we want to write to a string
	ds->write_data=0; //  and reset

	ds->compact=0;
	ds->strict=0;
	for(int i=0;(size_t)i<argc;i++) // check string flags in args
	{
		const char *cp=core_get_string(env,argv[i]);
		if( strcmp(cp,"djon")==0 ) { write_djon=1; }
		if( strcmp(cp,"compact")==0 ) { ds->compact=1; }
		if( strcmp(cp,"strict")==0 ) { ds->strict=1; }
		free((void*)cp);
	}

	if(write_djon)
	{
		djon_write_djon(ds,ds->parse_value);
	}
	else
	{
		djon_write_json(ds,ds->parse_value);
	}
	
	if(ds->write_data)
	{
		ret=core_string( env , ds->write_data );
	}
	
	free(ds->write_data); // and free write buffer
	ds->write_data=0;


	return ret;
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

		{		"djoncore"	,	djon_core_new	},

		{0,0}
	};
	return core_export_functions(env,exports,funcs,0);
}

