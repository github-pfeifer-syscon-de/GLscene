PKGCONFIG = pkgconf
GLIB_COMPILE_RESOURCES = $(shell $(PKGCONFIG) --variable=glib_compile_resources gio-2.0)

GLSCENE_RESOURCE_XML=glsceneapp.gresource.xml
GLSCENE_RESOURCES=$(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=. --generate-dependencies $(GLSCENE_RESOURCE_XML))

all:../src/resources.c

../src/resources.c: $(GLSCENE_RESOURCE_XML) $(GLSCENE_RESOURCES)
	$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(dir $<) --generate-source $<

# The desktop files
desktopdir = $(datadir)/applications
desktop_DATA=glscene.desktop

# application icon
appicondir = $(datadir)/icons/hicolor/64x64/apps
appicon_DATA = glscene.png
appdir_icon=$(appicondir)/$(appicon_DATA)

EXTRA_DIST = $(GLSCENE_RESOURCES) $(GLSCENE_RESOURCE_XML) $(desktop_in_files) $(appicon_DATA)

clean-local:
