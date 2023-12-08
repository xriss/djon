{
  "targets": [
    {
        "target_name": "djon_core",
        "sources": [ "djon_core.cpp" ], 
        "include_dirs" : [
            "<!(node -p \"require('node-addon-api').include_dir\")",
            "../c"
        ],
        'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ]
    }
  ],
  'dependencies': [
    "<!(node -p \"require('node-addon-api').targets\"):node_addon_api_maybe",
  ],
  'conditions': [
    ['OS=="mac"', {
        'cflags+': ['-fvisibility=hidden'],
        'xcode_settings': {
          'GCC_SYMBOLS_PRIVATE_EXTERN': 'YES', # -fvisibility=hidden
        }
    }]
  ]
}
