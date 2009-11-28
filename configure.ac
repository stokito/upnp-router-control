AC_INIT([upnp-router-control], [0.1], [https://launchpad.net/upnp-router-control])

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

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

IT_PROG_INTLTOOL([0.40.0])
AM_GLIB_GNU_GETTEXT


#### Check for GTK+ >= 2.14 ####
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.16,,
	AC_MSG_ERROR([GTK+-2.6 is required to compile upnp-router-control]))
	
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

#### Check for gupnp1.0 ####
PKG_CHECK_MODULES(GUPNP, gupnp-1.0,,
	AC_MSG_ERROR([libgupnp-1.0 is required to compile upnp-router-control]))
	
AC_SUBST(GUPNP_CFLAGS)
AC_SUBST(GUPNP_LIBS)

AC_CONFIG_FILES([
Makefile
src/Makefile
data/Makefile
po/Makefile.in
])

AC_OUTPUT

echo
echo -e "Now type 'make' to compile upnp-router-control,\nthen type 'sudo make install' to install it."
