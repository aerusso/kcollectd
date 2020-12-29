subdirs(en)

configure_file(man-kcollectd.1.docbook.in man-kcollectd.1.docbook)
kdoctools_create_manpage(${CMAKE_CURRENT_BINARY_DIR}/man-kcollectd.1.docbook 1 INSTALL_DESTINATION ${KDE_INSTALL_MANDIR})
