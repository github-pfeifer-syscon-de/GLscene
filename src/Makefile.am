## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)."\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$(GENERICGLM_CFLAGS) \
	$(GTKMM_CFLAGS) \
	$(GLIBMM_CFLAGS) \
	$(GTHREAD_CFLAGS) \
	$(EPOXY_CFLAGS)

AM_CFLAGS= \
	-Wall

AM_CXXFLAGS =\
	-Wall

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
	Row.hpp \
	TextContext.cpp \
	TextContext.hpp \
	GlSceneApp.cpp \
	GlSceneApp.hpp \
	SmokeContext.cpp \
	SmokeContext.hpp \
	Row.cpp

glscene_LDFLAGS =

glscene_LDADD = $(GENERICGLM_LIBS) \
	$(GLIBMM_LIBS) \
	$(GTKMM_LIBS) \
	$(GTHREAD_LIBS) \
	$(EPOXY_LIBS) 

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(pkgdatadir)

