INCLUDES =                                                  \
       -DDATADIR=\""$(datadir)"\"					        \
       -DPREFIX=\""$(prefix)"\" 			                \
	   -DPACKAGE_DATADIR=\""$(datadir)/upnp-router-control"\"  \
	   -DLOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

AM_CFLAGS = 						\
	@GTK_CFLAGS@					\
    @GSSDP_LIBS@                    \
	@GUPNP_CFLAGS@                  \
	@LIBCURL_CFLAGS@

bin_PROGRAMS = upnp-router-control

upnp_router_control_SOURCES = \
       urc-main.c \
       urc-gui.c \
       urc-gui.h \
       urc-upnp.c \
       urc-upnp.h \
       urc-graph.c \
       urc-graph.h

upnp_router_control_LDADD = @GTK_LIBS@ @INTLLIBS@ @GSSDP_LIBS@ @GUPNP_LIBS@ @LIBCURL_LIBS@ $(LIBCURL_LIBS) -lm

MAINTAINERCLEANFILES =				\
	*~			      				\
	Makefile.in

AM_CPPFLAGS = $(LIBCURL_CFLAGS) 
