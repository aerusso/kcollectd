project (kcollectd)

set(KCOLLECTD_ICONS_PNG
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/16-apps-kcollectd.png
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/22-apps-kcollectd.png
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/32-apps-kcollectd.png
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/48-apps-kcollectd.png
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/64-apps-kcollectd.png
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/128-apps-kcollectd.png
)

set(KCOLLECTD_ICONS_SVG
${CMAKE_CURRENT_SOURCE_DIR}/icons/sc-apps-kcollectd.svgz
)


add_executable(kcollectd
  graph.cc
  gui.cc
  kcollectd.cc
  misc.cc
  rrd_interface.cc
  timeaxis.cc)
set(rrd_LIBRARIES rrd)

kde_target_enable_exceptions(kcollectd PRIVATE)

target_link_libraries(kcollectd
PUBLIC
  KF5::XmlGui
  KF5::WidgetsAddons
  KF5::IconThemes
  KF5::I18n
  Qt5::Core
  Qt5::Widgets
  Qt5::Gui
  ${Boost_LIBRARIES}
  ${rrd_LIBRARIES}
)
install(TARGETS kcollectd  ${INSTALL_TARGETS_DEFAULT_ARGS})

# desktop-file
install(FILES kcollectd.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})

# mime-type
# for KDE 4.2 we require at least version 0.23
set(SHARED_MIME_INFO_MINIMUM_VERSION "0.23")
find_package(SharedMimeInfo REQUIRED)
install(FILES kcollectd.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})
