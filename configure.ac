AC_INIT([upnp-router-control],
        [0.2],
        [https://bugs.launchpad.net/upnp-router-control/+filebug],
        [upnp-router-control])

AM_CONFIG_HEADER(config.h)
AC_CONFIG_SRCDIR(src/urc-main.c)
AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE([enable])
# Use AM_SILENT_RULES if present
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl make sure we keep ACLOCAL_FLAGS around for maintainer builds to work
AC_SUBST(ACLOCAL_AMFLAGS, "\${ACLOCAL_FLAGS}")

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

CFLAGS="${CFLAGS} -Wall"

dnl Initialize intltool
GETTEXT_PACKAGE=upnp-router-control
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])

IT_PROG_INTLTOOL(0.40.0)
AM_GLIB_GNU_GETTEXT

AC_SEARCH_LIBS([ceil], [m])

AC_ARG_ENABLE(libcurl,
              [  --disable-libcurl      compile UPnP Router Control without icon download support],
              [libcurl=$enableval],
              [libcurl="yes"])


#### Check for GTK+ 3 ####
PKG_CHECK_MODULES(GTK, gtk+-3.0,,
	AC_MSG_ERROR([GTK+-3 is required to compile upnp-router-control]))

AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

#### Check for gssdp1.0 ####
PKG_CHECK_MODULES(GSSDP, gssdp-1.0,,
	AC_MSG_ERROR([libgssdp-1.0 is required to compile upnp-router-control]))

AC_SUBST(GSSDP_CFLAGS)
AC_SUBST(GSSDP_LIBS)

#### Check for gupnp1.0 ####
PKG_CHECK_MODULES(GUPNP, gupnp-1.0,,
	AC_MSG_ERROR([libgupnp-1.0 is required to compile upnp-router-control]))

AC_SUBST(GUPNP_CFLAGS)
AC_SUBST(GUPNP_LIBS)

#### Check for libcurl ####
if test "$enable_libcurl" = "no"
then
   echo "Disabling libcurl support"
else
	PKG_CHECK_MODULES(LIBCURL, libcurl,
	HAVE_LIBCURL="yes", HAVE_LIBCURL="no")

	if test "x$HAVE_LIBCURL" = "xyes"
	then
	   AC_DEFINE(HAVE_LIBCURL, 1, [libcurl available])
	fi
fi

AM_CONDITIONAL(LIBCURL, test -n "$LIBCURL_LIBS")

AC_SUBST(LIBCURL_CFLAGS)
AC_SUBST(LIBCURL_LIBS)

AC_CONFIG_FILES([
Makefile
src/Makefile
data/Makefile
po/Makefile.in
])

AC_OUTPUT

echo
echo -e "Now type 'make' to compile upnp-router-control,\nthen type 'sudo make install' to install it."
