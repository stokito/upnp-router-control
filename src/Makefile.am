INCLUDES =                                                  \
       -DDATADIR=\""$(datadir)"\"					        \
       -DPREFIX=\""$(prefix)"\" 			                \
	   -DPACKAGE_DATADIR=\""$(datadir)/upnp-router-control"\"  \
	   -DLOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"
	   
AM_CFLAGS = 						\
	@GTK_CFLAGS@					\
	@GUPNP_CFLAGS@

bin_PROGRAMS = upnp-router-control

upnp_router_control_SOURCES = \
       urc-main.c \
       urc-gui.c \
       urc-gui.h \
       urc-upnp.c \
       urc-upnp.h

upnp_router_control_LDADD = @GTK_LIBS@ @INTLLIBS@ @GUPNP_LIBS@

MAINTAINERCLEANFILES =				\
	*~			      				\
	Makefile.in
