subdir('icons')

desktop = meson.project_name() + '.desktop'

# Desktop files
i18n.merge_file(
  type: 'desktop',
  input: desktop + '.in',
  output: desktop,
  po_dir: po_dir,
  install: true,
  install_dir: join_paths(data_dir, 'applications')
)

# Man file
install_man('upnp-router-control.1')

# Appdata files
appdata = i18n.merge_file(
  'appdata',
  input: '@0@.appdata.xml.in'.format(application_id),
  output: '@0@.appdata.xml'.format(application_id),
  po_dir: po_dir,
  install: true,
  install_dir: join_paths(data_dir, 'metainfo')
)

