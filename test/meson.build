
incSrcTest = include_directories('../src'
                  , '..')

fft_test = executable('fft_test'
    , ['../src/Capture.cpp'
      , '../src/Fft.cpp'
      , '../src/ChunkedArray.cpp'
      , 'CooleyTukey.cpp'
      , 'fft_test.cpp']
    , include_directories: incSrcTest
    , dependencies: deps)
test('fft_test ', fft_test)

# load_test left for manual test

load_test = executable('load_test'
                      , ['../src/ObjLoader.cpp'
                         , 'load_test.cpp']
                      , include_directories: incSrcTest
                      , dependencies: deps)

# patest left for manual test

pa_test = executable('pa_test'
    , ['../src/Pulse.cpp'
      , '../src/ChunkedArray.cpp'
      , 'patest.cpp']
    , include_directories: incSrcTest
    , dependencies: deps)

