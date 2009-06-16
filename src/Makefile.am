INCLUDES =                                                  \
       -DDATADIR=\""$(datadir)"\"					        \
       -DPREFIX=\""$(prefix)"\" 			                \
	   -DPACKAGE_DATADIR=\""$(datadir)/upnp-port-mapper"\"  \
	   -DLOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"
	   
AM_CFLAGS = 						\
	@GTK_CFLAGS@					\
	@GLADE_CFLAGS@				    \
	@GUPNP_CFLAGS@

bin_PROGRAMS = upnp-port-mapper

upnp_port_mapper_SOURCES = \
       upnp-port-mapper.c \
       upnp-port-mapper.h

upnp_port_mapper_LDADD = @GTK_LIBS@ @INTLLIBS@ @GLADE_LIBS@ @GUPNP_LIBS@

MAINTAINERCLEANFILES =				\
	*~			      				\
	Makefile.in
