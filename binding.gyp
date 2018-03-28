{
 "variables": {
    "root%": "/Users/suresh/workspace/c++"
 },
 "targets": [ 
   { 
     "target_name": "regex_replace",
     "sources": [ "regex_replace.cc" ],
     "xcode_settings": {
        "OTHER_CFLAGS": [
          "-std=c++14",
          "-stdlib=libc++"
        ],
      },
     "include_dirs": [ "<!(node -e \"require('nan')\")", "<@(root)/boost_1_66_0/include", "<@(root)/openssl-1.0.2n/include" ],
     "link_settings": {
        "libraries": [
          "-L<@(root)/boost_1_66_0/lib",
          "-L<@(root)/openssl-1.0.2n/lib"
        ],
        "ldflags": [
            "-L<@(root)/boost_1_66_0/lib",
            "-L<@(root)/openssl-1.0.2n/lib",
            "-Wl,-rpath,<@(root)/boost_1_66_0/lib",
            "-Wl,-rpath,<@(root)/openssl-1.0.2n/lib"
        ]
      },
     "cflags_cc!": [ "-fno-rtti", "-fno-exceptions" ],
     "cflags!": [ "-fno-exceptions" ],
     "conditions": [
                [ 'OS=="mac"', {
                    "xcode_settings": {
                        'OTHER_CPLUSPLUSFLAGS' : ['-std=c++11','-stdlib=libc++', '-v'],
                        'OTHER_LDFLAGS': ['-stdlib=libc++'],
                        'MACOSX_DEPLOYMENT_TARGET': '10.7',
                        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
                    }
                }]
            ]
   } 
 ]
}
