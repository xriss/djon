
#include "napi.h"

#include "djon.h"

class DjonCore : public Napi::ObjectWrap<DjonCore> {
  public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    DjonCore(const Napi::CallbackInfo& info);
    ~DjonCore();

  private:
    djon_state *ds;
    Napi::Value location(const Napi::CallbackInfo& info);
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
/*
  Napi::Env env = info.Env();
  // ...
  Napi::Number value = info[0].As<Napi::Number>();
  this->_value = value.DoubleValue();
*/
}

DjonCore::~DjonCore() {
}


Napi::Value DjonCore::location(const Napi::CallbackInfo& info){
    Napi::Env env = info.Env();
    return Napi::Number::New(env, -1);
}

Napi::Value DjonCore::get(const Napi::CallbackInfo& info){
    Napi::Env env = info.Env();
    return Napi::Number::New(env, -2);
}

Napi::Value DjonCore::set(const Napi::CallbackInfo& info){
    Napi::Env env = info.Env();
    return Napi::Number::New(env, -3);
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
