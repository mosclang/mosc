#!/bin/sh
rm -rf build
if [ "$1" = "run" ]; then
    emcmake cmake -Bbuild -H. -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=../runner/public \
    -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=../runner/public \
    -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=../runner/public -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_C_FLAGS=-fdebug-compilation-dir=. \
    -DCMAKE_CXX_FLAGS=-fdebug-compilation-dir=. 
    
    emmake make -C build
    cd runner
    npm run start
else
    emcmake cmake -Bbuild -H.
    emmake make -C build
    cp build/mosc-try.js build/mosc-try.wasm ../doc/static/
    if [ $1 = "deploy" ]; then
        echo "Deploying the documentation also"
        cd  ../doc
        sh ./deploy
    fi
fi
