
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

# patest left for manual test

pa_test = executable('pa_test'
    , ['../src/Pulse.cpp'
      , '../src/ChunkedArray.cpp'
      , 'patest.cpp']
    , include_directories: incSrcTest
    , dependencies: deps)

