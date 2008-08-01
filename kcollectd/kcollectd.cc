/*
 * This file is part of the source of kcollectd, a viewer for
 * rrd-databases created by collectd
 * 
 * Copyright (C) 2008 M G Berberich
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string>
#include <vector>
#include <set>
#include <sstream>
#include <exception>

#include <boost/filesystem.hpp>

#include <kcmdlineargs.h>
#include <kapplication.h>
#include <klistview.h>
#include <kmessagebox.h>
#include <klocale.h>

#include "rrd_interface.h"
#include "gui.h"


#ifndef RRD_BASEDIR
# define RRD_BASEDIR "/var/lib/collectd/rrd"
#endif

void get_datasources(const std::string &rrdfile, const std::string &info,
      KListViewItem *item)
{
  std::set<std::string> datasources;
  get_dsinfo(rrdfile, datasources);

  for(std::set<std::string>::iterator i=datasources.begin();
      i != datasources.end(); ++i){
    new KListViewItem(item, *i, info + *i, rrdfile);
  }
}

void get_rrds(const boost::filesystem::path rrdpath, KListView *listview)
{
  using namespace boost::filesystem;
  
  const directory_iterator end_itr;
  for (directory_iterator host(rrdpath); host != end_itr; ++host ) {
    if (is_directory(*host)) {
      KListViewItem *hostitem = new KListViewItem(listview, host->leaf());
      hostitem->setSelectable(false);
      for (directory_iterator sensor(*host); sensor != end_itr; ++sensor ) {
	if (is_directory(*sensor)) {
	  KListViewItem *sensoritem = new KListViewItem(hostitem, 
		sensor->leaf());
	  sensoritem->setSelectable(false);
	  for (directory_iterator rrd(*sensor); rrd != end_itr; ++rrd ) {
	    if (is_regular(*rrd) && extension(*rrd) == ".rrd") {
	      KListViewItem *rrditem = new KListViewItem(sensoritem, 
		    basename(*rrd));
	      rrditem->setSelectable(false);
	      std::ostringstream info;
	      info << host->leaf() << " . "
		   << sensor->leaf() << " . "
		   << basename(*rrd) << " . ";
	      get_datasources(rrd->string(), info.str(), rrditem);
	    }
	  }
	}
      }
    }
  }
}

int main(int argc, char **argv)
{
  using namespace boost::filesystem;

  std::vector<std::string> rrds;
  
  KCmdLineArgs::init(argc, argv, "kcollectd", i18n("KCollectd"), 
	i18n("Viewer for Collectd-databases"), "0.1");
  KApplication a;
  KCollectdGui gui;
  
  try {
    get_rrds(RRD_BASEDIR, gui.listview);
  } 
  catch(basic_filesystem_error<path> &e) {
    KMessageBox::error(0, QString(i18n("Failed to read collectd-structure at "
		      "\'%1\'\nTerminating.")).arg(RRD_BASEDIR));
    exit(1);
  } 
  catch(bad_rrdinfo &e) {
    KMessageBox::error(0, e.what());
    exit(2);
  }
  
  a.setMainWidget(&gui);
  gui.show();
  return a.exec();
}
