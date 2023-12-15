
#include "napi.h"

#define DJON_C 1
#include "djon.h"

class DjonCore : public Napi::ObjectWrap<DjonCore> {
	public:
		static Napi::Object Init(Napi::Env env, Napi::Object exports);
		DjonCore(const Napi::CallbackInfo& info);
		~DjonCore();

	private:
		djon_state *ds;
		Napi::Value location(const Napi::CallbackInfo& info);
		Napi::Value get_value(const Napi::Env& env,int idx);
		int         set_value(const Napi::Env& env,const Napi::Value& val);
		void        set_fix(int idx,char *base);
		Napi::Value get(const Napi::CallbackInfo& info);
		Napi::Value set(const Napi::CallbackInfo& info);
		Napi::Value load(const Napi::CallbackInfo& info);
		Napi::Value save(const Napi::CallbackInfo& info);
};

Napi::Object DjonCore::Init(Napi::Env env, Napi::Object exports) {
	// This method is used to hook the accessor and method callbacks
	Napi::Function func = DefineClass(env, "DjonCore", {
		InstanceMethod<&DjonCore::location>("location", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
		InstanceMethod<&DjonCore::get>("get", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
		InstanceMethod<&DjonCore::set>("set", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
		InstanceMethod<&DjonCore::load>("load", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
		InstanceMethod<&DjonCore::save>("save", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
	});

	Napi::FunctionReference* constructor = new Napi::FunctionReference();

	// Create a persistent reference to the class constructor. This will allow
	// a function called on a class prototype and a function
	// called on instance of a class to be distinguished from each other.
	*constructor = Napi::Persistent(func);
	exports.Set("DjonCore", func);

	// Store the constructor as the add-on instance data. This will allow this
	// add-on to support multiple instances of itself running on multiple worker
	// threads, as well as multiple instances of itself running in different
	// contexts on the same thread.
	//
	// By default, the value set on the environment here will be destroyed when
	// the add-on is unloaded using the `delete` operator, but it is also
	// possible to supply a custom deleter.
	env.SetInstanceData<Napi::FunctionReference>(constructor);


	return exports;
}

DjonCore::DjonCore(const Napi::CallbackInfo& info) :
	Napi::ObjectWrap<DjonCore>(info) {
	ds=djon_setup();
}

DjonCore::~DjonCore() {
	if(ds)
	{
		djon_clean(ds);
	}
	ds=0;
}


Napi::Value DjonCore::location(const Napi::CallbackInfo& info){
	Napi::Env env = info.Env();
	Napi::Array a = Napi::Array::New(env,3);
	a[(uint32_t)0]=Napi::Number::New(env,ds->error_line);
	a[(uint32_t)1]=Napi::Number::New(env,ds->error_char);
	a[(uint32_t)2]=Napi::Number::New(env,ds->error_idx);
	return a;
}

Napi::Value DjonCore::get_value(const Napi::Env& env,int idx){
djon_value *v=djon_get(ds,idx);
int vi=0;
int ki=0;
	switch(v->typ&DJON_TYPEMASK)
	{
		case DJON_ARRAY:
			vi=v->val;
			while( vi )
			{
				get_value( env , vi );
				vi=djon_get(ds,vi)->nxt;
			}
		break;
		case DJON_OBJECT:
			ki=v->key;
			while( ki )
			{
				get_value( env , ki );
				get_value( env , djon_get(ds,ki)->val );
				ki=djon_get(ds,ki)->nxt;
			}
		break;
		case DJON_STRING:
		break;
		case DJON_NUMBER:
		break;
		case DJON_BOOL:
		break;
		default:
			return env.Null();
		break;
	}
	
	return env.Null();
}

int DjonCore::set_value(const Napi::Env& env,const Napi::Value& val){
int di=0;
djon_value *dv=0;
	if( val.IsArray() )
	{
		di=djon_alloc(ds); if(!di) { Napi::Error::Fatal( "djon.core", "out of memory" ); }
		dv=djon_get(ds,di);
		dv->typ=DJON_ARRAY;

		Napi::Array a = val.As<Napi::Array>();
		int len=a.Length();
		int li=0;
		for(int i=0;i<len;i++)
		{
			int vi=this->set_value(env,a[i]);

			dv=djon_get(ds,di); // realloc safe
			if( dv->val==0) // first
			{
				dv->val=vi;
			}
			else // chain
			{
				djon_get(ds,li)->nxt=vi;
				djon_get(ds,vi)->prv=li;
			}
			li=vi;
		}
		return di;
	}
	else
	if( val.IsObject() )
	{
		di=djon_alloc(ds); if(!di) { Napi::Error::Fatal( "djon.core", "out of memory" ); }
		dv=djon_get(ds,di);
		dv->typ=DJON_OBJECT;

		Napi::Object o = val.As<Napi::Object>();
		Napi::Array  a = o.GetPropertyNames();
		int len=a.Length();
		int li=0;
		for(int i=0;i<len;i++)
		{
			Napi::Value k=a[i];
			Napi::Value v=o.Get(k);
//			printf(" %s = %s ",k.As<Napi::String>().Utf8Value().c_str(),v.As<Napi::String>().Utf8Value().c_str());

			int ki=this->set_value(env,k);
			int vi=this->set_value(env,v);
			djon_get(ds,ki)->val=vi; // key to value

			dv=djon_get(ds,di); // realloc safe
			if( dv->key==0) // first
			{
				dv->key=ki;
			}
			else // chain
			{
				djon_get(ds,li)->nxt=ki;
				djon_get(ds,ki)->prv=li;
			}
			li=ki;

		}
		return di;
	}
	else
	if( val.IsString() )
	{
		Napi::String ns=val.As<Napi::String>();
		std::string s=ns.Utf8Value();
		const char *cp=s.c_str();
		int len=s.length();
		int p=djon_write_data(ds,cp,len+1); // save string (hax) and remember its offset into data chunk

		di=djon_alloc(ds); if(!di) { Napi::Error::Fatal( "djon.core", "out of memory" ); }
		dv=djon_get(ds,di);
		dv->typ=DJON_STRING;
		dv->str=((char *)(0))+p; // we will need to add base memory address when we finish
		dv->len=len;
		return di;
	}
	else
	if( val.IsNumber() )
	{
		di=djon_alloc(ds); if(!di) { Napi::Error::Fatal( "djon.core", "out of memory" ); }
		dv=djon_get(ds,di);
		dv->typ=DJON_NUMBER;
		dv->num=val.As<Napi::Number>().DoubleValue();
		return di;
	}
	else
	if( val.IsBoolean() )
	{
		di=djon_alloc(ds); if(!di) { Napi::Error::Fatal( "djon.core", "out of memory" ); }
		dv=djon_get(ds,di);
		dv->typ=DJON_BOOL;
		dv->num=val.As<Napi::Boolean>().Value()?1.0:0.0; // bool to double
		return di;
	}
	else // everything else is a null
	{
		di=djon_alloc(ds); if(!di) { Napi::Error::Fatal( "djon.core", "out of memory" ); }
		dv=djon_get(ds,di);
		dv->typ=DJON_NULL;
		return di;
	}
	return 0;
}

void DjonCore::set_fix(int idx,char *base){
djon_value *v=djon_get(ds,idx);
int vi=0;
int ki=0;
	switch(v->typ&DJON_TYPEMASK)
	{
		case DJON_ARRAY:
			vi=v->val;
			while( vi )
			{
				set_fix( vi , base );
				vi=djon_get(ds,vi)->nxt;
			}
		break;
		case DJON_OBJECT:
			ki=v->key;
			while( ki )
			{
				set_fix( ki , base );
				set_fix( djon_get(ds,ki)->val , base );
				ki=djon_get(ds,ki)->nxt;
			}
		break;
		case DJON_STRING:
			v->str=((long)v->str)+base;
		break;
	}
}

Napi::Value DjonCore::get(const Napi::CallbackInfo& info){
	Napi::Env env = info.Env();
	return get_value(env,ds->parse_value);
}

Napi::Value DjonCore::set(const Napi::CallbackInfo& info){
	Napi::Env env = info.Env();
	ds->write_data=0;
	ds->parse_value=set_value(env,info[0]);
	// keep string data and fix all the pointers
	ds->data=ds->write_data;
	ds->data_len=ds->write_size;
	ds->write_data=0;
	set_fix(ds->parse_value,ds->data);
	return Napi::Number::New(env, 0);
}

Napi::Value DjonCore::load(const Napi::CallbackInfo& info){
	Napi::Env env = info.Env();
	return Napi::Number::New(env, -4);
}

Napi::Value DjonCore::save(const Napi::CallbackInfo& info){
	Napi::Env env = info.Env();
	return Napi::Number::New(env, -5);
}

// Initialize native add-on
Napi::Object Init (Napi::Env env, Napi::Object exports) {
	DjonCore::Init(env, exports);
	return exports;
}


// Register and initialize native add-on
NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
