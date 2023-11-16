#include <nan.h>

NAN_METHOD(aString);


NAN_METHOD(aString) {
    info.GetReturnValue().Set(Nan::New("This is a thing.").ToLocalChecked());
}

using v8::FunctionTemplate;

NAN_MODULE_INIT(InitAll) {
  Nan::Set(target, Nan::New("aString").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(aString)).ToLocalChecked());
}

NODE_MODULE(djon_core, InitAll)


