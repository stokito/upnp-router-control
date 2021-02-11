icons = \
	apps_16x16_upnp-router-control.png \
	apps_22x22_upnp-router-control.png \
	apps_24x24_upnp-router-control.png \
	apps_32x32_upnp-router-control.png \
	apps_48x48_upnp-router-control.png \
	apps_scalable_upnp-router-control.svg \
	$(NULL)

noinst_DATA = \
	apps_16x16_upnp-router-control.svg \
	apps_22x22_upnp-router-control.svg \
	apps_24x24_upnp-router-control.svg \
	apps_32x32_upnp-router-control.svg \
	apps_48x48_upnp-router-control.svg \
	$(NULL)

gladedir = $(datadir)/upnp-router-control
glade_DATA = \
    upnp-router-control.ui

glade_headerdir = $(datadir)/upnp-router-control
glade_header_DATA = \
    upnp-router-control-headermenu.ui

# AppData
@INTLTOOL_XML_RULE@
appdatadir = $(datadir)/appdata
appdata_in_files = upnp-router-control.appdata.xml.in
appdata_DATA = $(appdata_in_files:.xml.in=.xml)

man_MANS = upnp-router-control.1

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_in_files = upnp-router-control.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

EXTRA_DIST = \
      $(icon_DATA)      \
      $(noinst_DATA)   \
      $(glade_DATA)     \
      $(glade_header_DATA)	\
      $(desktop_DATA)   \
      $(appdata_DATA)   \
      $(man_MANS)
      $(NULL)

DISTCLEANFILES = \
    $(desktop_DATA) \
    $(appdata_DATA)


###############################################################################

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

install-icons:
	for icon in $(icons); do \
		CONTEXT=`echo $$icon | cut -d_ -f1`; \
		SIZE=`echo $$icon | cut -d_ -f2`; \
		ICONFILE=`echo $$icon | cut -d_ -f3`; \
		mkdir -p $(DESTDIR)$(datadir)/icons/hicolor/$$SIZE/$$CONTEXT; \
		$(INSTALL_DATA) $(srcdir)/$$icon $(DESTDIR)$(datadir)/icons/hicolor/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done;

uninstall-icons:
	-for icon in $(icons); do \
		CONTEXT=`echo $$icon | cut -d_ -f1`; \
		SIZE=`echo $$icon | cut -d_ -f2`; \
		ICONFILE=`echo $$icon | cut -d_ -f3`; \
		rm -f $(DESTDIR)$(datadir)/icons/hicolor/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done;

clean-local:
	rm -f *~

install-data-local: install-icons update-icon-cache
uninstall-local: uninstall-icons update-icon-cache

