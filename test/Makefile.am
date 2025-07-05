## Process this file with automake to produce Makefile.in


TESTS = fft_test patest

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)."\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$(GENERICGLM_CFLAGS) \
	$(ALSA_CFLAGS) \
	-I../src

AM_CFLAGS= \
	-Wpedantic -Wall -Wconversion

AM_CXXFLAGS = \
	-Wpedantic -Wall -Wconversion

check_PROGRAMS = fft_test patest

fft_test_LDADD =  \
	../src/Capture.o \
	../src/Fft.o \
	../src/ChunkedArray.o \
	$(GENERICGLM_LIBS) \
	$(ALSA_LIBS)

fft_test_SOURCES = \
    fft_test.cpp

patest_LDADD =  \
	../src/Pulse.o \
	../src/ChunkedArray.o \
	$(GENERICIMG_LIBS) \
	$(ALSA_LIBS)

patest_SOURCES = \
    patest.cpp
