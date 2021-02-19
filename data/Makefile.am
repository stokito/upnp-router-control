icons = \
	apps_16x16_org.upnproutercontrol.UPnPRouterControl.png      \
	apps_22x22_org.upnproutercontrol.UPnPRouterControl.png      \
	apps_24x24_org.upnproutercontrol.UPnPRouterControl.png      \
	apps_32x32_org.upnproutercontrol.UPnPRouterControl.png      \
	apps_48x48_org.upnproutercontrol.UPnPRouterControl.png      \
	apps_scalable_org.upnproutercontrol.UPnPRouterControl.svg   \
	$(NULL)

noinst_DATA = \
	apps_16x16_org.upnproutercontrol.UPnPRouterControl.svg      \
	apps_22x22_org.upnproutercontrol.UPnPRouterControl.svg      \
	apps_24x24_org.upnproutercontrol.UPnPRouterControl.svg      \
	apps_32x32_org.upnproutercontrol.UPnPRouterControl.svg      \
	apps_48x48_org.upnproutercontrol.UPnPRouterControl.svg      \
	apps_64x64_org.upnproutercontrol.UPnPRouterControl.svg      \
	$(NULL)

# AppData
@INTLTOOL_XML_RULE@
appdatadir = $(datadir)/metainfo
appdata_in_files = upnp-router-control.appdata.xml.in
appdata_DATA = $(appdata_in_files:.xml.in=.xml)

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_in_files = upnp-router-control.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

man_MANS = upnp-router-control.1

EXTRA_DIST =                \
    $(icon_DATA)            \
    $(noinst_DATA)          \
    $(desktop_in_files)     \
    $(appdata_in_files)     \
    $(man_MANS)             \
    $(NULL)

DISTCLEANFILES =        \
    $(desktop_DATA)     \
    $(appdata_DATA)     \
    $(NULL)

CLEANFILES =            \
    $(desktop_DATA)     \
    $(appdata_DATA)     \
    $(NULL)

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
		ICONFILE=org.upnproutercontrol.UPnPRouterControl
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

