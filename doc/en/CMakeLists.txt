configure_file(index.docbook.in index.docbook)
kdoctools_create_handbook(${CMAKE_CURRENT_BINARY_DIR}/index.docbook INSTALL_DESTINATION ${HTML_INSTALL_DIR}/en SUBDIR kcollectd)
