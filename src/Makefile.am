## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)."\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$(GENERICGLM_CFLAGS) \
	$(GTHREAD_CFLAGS) \
	$(ALSA_CFLAGS) \
	$(EPOXY_CFLAGS)

bin_PROGRAMS = glscene

glscene_LDFLAGS =

# this is a no-tech approach,
#  if you don't like gcc/clang, this should also work without these options
#  most options originate from https://github.com/cpp-best-practices/cmake_template/blob/main/cmake/Hardening.cmake
#    -W... the usual warings
#    GLIBC++ Assertions (vector[], string[], ...) enabled
#    g++/clang -fstack-protector-strong enabled
#    g++/clang -fcf-protection enabled
#    undefined behaviour sanitizer
if RELEASE
glscene_CXXFLAGS = -mtune=native -march=native -O3
else
glscene_CFLAGS = -Wall
glscene_CXXFLAGS = -Wall \
        -Wconversion \
        -Wpedantic \
        -D_GLIBCXX_ASSERTIONS \
        -fstack-protector-strong \
        -fcf-protection \
        -fsanitize=undefined
glscene_LDFLAGS += -fsanitize=undefined
endif

glscene_SOURCES = \
	resources.c \
	Bump.cpp \
	Bump.hpp \
	GlPlaneView.cpp \
	GlPlaneView.hpp \
	GlSceneWindow.cpp \
	GlSceneWindow.hpp \
	PlaneContext.cpp \
	PlaneContext.hpp \
	PlaneGeometry.cpp \
	PlaneGeometry.hpp \
	Row.cpp \
	Row.hpp \
	TextContext.cpp \
	TextContext.hpp \
	GlSceneApp.cpp \
	GlSceneApp.hpp \
	SmokeContext.cpp \
	SmokeContext.hpp \
	ChunkedArray.cpp \
	ChunkedArray.hpp \
	Pulse.cpp \
	Pulse.hpp \
	Fft.cpp \
	Fft.hpp \
	PrefDialog.cpp \
	PrefDialog.hpp \
	SpectrumPlot.cpp \
	SpectrumPlot.hpp \
	Capture.cpp \
	Capture.hpp

glscene_LDADD = $(GENERICGLM_LIBS) \
	$(GLIBMM_LIBS) \
	$(GTHREAD_LIBS) \
	$(ALSA_LIBS) \
	$(EPOXY_LIBS)

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(pkgdatadir)

