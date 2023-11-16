{
  "targets": [
    {
        "target_name": "djon_core",
        "sources": [ "djon_core.cpp" ], 
        "include_dirs" : ["<!(node -e \"require('nan')\")","../c"]
    }
  ]
}
