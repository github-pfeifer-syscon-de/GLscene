## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)."\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$(GENERICGLM_CFLAGS) \
	$(GTHREAD_CFLAGS) \
	$(ALSA_CFLAGS) \
	$(EPOXY_CFLAGS)

AM_CFLAGS= \
	-Wpedantic -Wall -Wconversion

AM_CXXFLAGS = \
	-Wpedantic -Wall -Wconversion

bin_PROGRAMS = glscene

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
	Capture.cpp \
	Capture.hpp

glscene_LDFLAGS =

glscene_LDADD = $(GENERICGLM_LIBS) \
	$(GLIBMM_LIBS) \
	$(GTHREAD_LIBS) \
	$(ALSA_LIBS) \
	$(EPOXY_LIBS)

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(pkgdatadir)

