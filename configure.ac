dnl Process this file with autoconf to produce a configure script.

AC_CONFIG_MACRO_DIR([m4])

AC_INIT([glscene],[0.1],[glscene@pfeifer-syscon.de])

AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11])

AM_SILENT_RULES([yes])

AX_CHECK_ENABLE_DEBUG([info], [DEBUG], [RELEASE], [IS-RELEASE])
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX([20], [noext], [optional])

LT_INIT

PKG_CHECK_MODULES(EPOXY,epoxy)

PKG_CHECK_MODULES(ALSA,[alsa, fftw3, libpulse, libpulse-mainloop-glib])

PKG_CHECK_MODULES(GTHREAD,gthread-2.0)

PKG_CHECK_MODULES([GENERICGLM], [genericglm >= 0.3.2])
PKG_CHECK_MODULES([GENERICIMG], [genericimg >= 0.4.5])

dnl don't repeat previous dependencies

dnl PKG_CHECK_MODULES(GLIBMM,glibmm-2.4)
dnl PKG_CHECK_MODULES(GTKMM, [gtkmm-3.0 ])

AC_CONFIG_FILES([
Makefile
src/Makefile
res/Makefile
test/Makefile
])
AC_OUTPUT
