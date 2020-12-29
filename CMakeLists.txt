cmake_minimum_required(VERSION 3.0)
cmake_policy(SET CMP0063 NEW)

project(kcollectd VERSION 0.11.99.0)
set(PROJECT_RELEASE_DATE 2020-07-13)

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(FindPkgConfig)
include(KDEInstallDirs)
include(KDECompilerSettings)
include(KDECMakeSettings)
include(FeatureSummary)
include(ECMInstallIcons)

find_package(Qt5 5.7.0 CONFIG REQUIRED Core Widgets Gui)

# Load the frameworks we need
find_package(KF5 REQUIRED COMPONENTS
  Config
  DocTools
  GuiAddons
  KIO
  I18n
  XmlGui
  IconThemes
  WidgetsAddons
)

find_package(Boost COMPONENTS filesystem system)

add_definitions(-Wall -g)

# Basedir for collectd
if(NOT RRD_BASEDIR)
  set(RRD_BASEDIR "/var/lib/collectd/rrd")
endif(NOT RRD_BASEDIR)
set(RRD_BASEDIR ${RRD_BASEDIR} CACHE PATH "default path to collectd-data")

# config.h
configure_file(config.h.in config.h)

include_directories(${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS} )

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

subdirs(kcollectd po doc)

