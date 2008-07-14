SUBDIRS = icons

# set the include path for X, qt and KDE
INCLUDES = $(all_includes)

# these are the headers for your project
noinst_HEADERS = graph.h gui.h rrd_interface.h labeling.h

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages: *.cc *.h
	$(XGETTEXT) *.cc *.h -o $(podir)/kcollectd.pot

KDE_ICON = AUTO

#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = kcollectd

# the application source, library search path, and link libraries
kcollectd_SOURCES = graph.cc gui.cc kcollectd.cc rrd_interface.cc labeling.cc
kcollectd_LDFLAGS = $(KDE_RPATH) $(all_libraries)
kcollectd_LDADD = $(LIB_KDEUI) -lboost_filesystem -lrrd

# this is where the desktop file will go 
shelldesktopdir   = $(kde_appsdir)/Utilities
shelldesktop_DATA = kcollectd.desktop


