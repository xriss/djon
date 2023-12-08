#include <nan.h>

NAN_METHOD(setup);
NAN_METHOD(clean);
NAN_METHOD(location);
NAN_METHOD(get);
NAN_METHOD(set);
NAN_METHOD(load);
NAN_METHOD(save);


NAN_METHOD(setup) {
    info.GetReturnValue().Set(Nan::New("This is a setup.").ToLocalChecked());
}
NAN_METHOD(clean) {
    info.GetReturnValue().Set(Nan::New("This is a clean.").ToLocalChecked());
}

NAN_METHOD(location) {
    info.GetReturnValue().Set(Nan::New("This is a location.").ToLocalChecked());
}

NAN_METHOD(get) {
    info.GetReturnValue().Set(Nan::New("This is a get.").ToLocalChecked());
}
NAN_METHOD(set) {
    info.GetReturnValue().Set(Nan::New("This is a set.").ToLocalChecked());
}

NAN_METHOD(load) {
    info.GetReturnValue().Set(Nan::New("This is a load.").ToLocalChecked());
}
NAN_METHOD(save) {
    info.GetReturnValue().Set(Nan::New("This is a save.").ToLocalChecked());
}

using v8::FunctionTemplate;

NAN_MODULE_INIT(InitAll) {
  Nan::Set(target, Nan::New("setup").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(setup)).ToLocalChecked());
  Nan::Set(target, Nan::New("clean").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(clean)).ToLocalChecked());

  Nan::Set(target, Nan::New("location").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(location)).ToLocalChecked());

  Nan::Set(target, Nan::New("get").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(get)).ToLocalChecked());
  Nan::Set(target, Nan::New("set").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(set)).ToLocalChecked());

  Nan::Set(target, Nan::New("load").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(load)).ToLocalChecked());
  Nan::Set(target, Nan::New("save").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(save)).ToLocalChecked());
}

NODE_MODULE(djon_core, InitAll)


