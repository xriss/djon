
#include "node_api.h"

extern napi_value djon_test(napi_env env, napi_callback_info info);


#define DJON_C 1
#include "djon.h"


#define NODE_API_CALL(env, call)                                  \
  do {                                                            \
    napi_status status = (call);                                  \
    if (status != napi_ok) {                                      \
      bool is_pending;                                            \
      napi_is_exception_pending((env), &is_pending);              \
      /* If an exception is already pending, don't rethrow it */  \
      if (!is_pending) {                                          \
        const char* message = "empty error message";              \
        napi_throw_error((env), NULL, message);                   \
      }                                                           \
      return NULL;                                                \
    }                                                             \
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


extern napi_value djon_test(napi_env env, napi_callback_info info)
{
  return core_create_double(env,123.4);
}


NAPI_MODULE_INIT() {
	const core_struct_functions funcs[]={

		{		"test"	,	djon_test	},

		{0,0}
	};
	return core_export_functions(env,exports,funcs,0);
}

