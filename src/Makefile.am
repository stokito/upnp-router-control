upnp_router_control_CFLAGS =    	\
    @GTK_CFLAGS@                    \
    @GSSDP_LIBS@                    \
    @GUPNP_CFLAGS@                  \
    -DDATADIR=\""$(datadir)"\"                              \
    -DPREFIX=\""$(prefix)"\"                                \
    -DPACKAGE_DATADIR=\""$(datadir)/upnp-router-control"\"  \
    -DLOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"       \
    $(NULL)

bin_PROGRAMS = upnp-router-control

RESOURCES =             \
    urc-resources.c     \
    urc-resources.h     \
    $(NULL)

upnp_router_control_SOURCES = \
    urc-main.c      \
    urc-gui.c       \
    urc-gui.h       \
    urc-upnp.c      \
    urc-upnp.h      \
    urc-graph.c     \
    urc-graph.h     \
    $(RESOURCES)    \
    $(NULL)

upnp_router_control_LDADD = @GTK_LIBS@ @INTLLIBS@ @GSSDP_LIBS@ @GUPNP_LIBS@ -lm

RESOURCES_DEP = $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies upnp-router-control.gresource.xml)

urc-resources.h urc-resources.c: upnp-router-control.gresource.xml $(RESOURCES_DEP)
	$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name urc $(srcdir)/upnp-router-control.gresource.xml

BUILT_SOURCES = 		\
	$(RESOURCES)		\
	$(NULL)

EXTRA_DIST =                            \
    upnp-router-control.gresource.xml   \
    upnp-router-control.ui              \
    upnp-router-control-headermenu.ui

CLEANFILES =            \
    $(BUILT_SOURCES)    \
    *~                  \
    $(NULL)

