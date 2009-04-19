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
#include <iostream>
#include <vector>
#include <set>
#include <sstream>
#include <exception>

#include <boost/filesystem.hpp>

#include <QTreeWidgetItem>

#include <KCmdLineArgs>
#include <KApplication>
#include <KMessageBox>
#include <KLocale>

// Qt4 #include "../config.h"

#include "rrd_interface.h"
#include "gui.h"


#ifndef RRD_BASEDIR
# define RRD_BASEDIR "/var/lib/collectd/rrd"
#endif

void get_datasources(const std::string &rrdfile, const std::string &info,
      QTreeWidgetItem *item)
{
  std::set<std::string> datasources;
  get_dsinfo(rrdfile, datasources);
 
  if (datasources.size() == 1) {
    item->setFlags(item->flags() | Qt::ItemIsSelectable);
    item->setText(1, (info + *datasources.begin()).c_str());
    item->setText(2, rrdfile.c_str());
    item->setText(3, (*datasources.begin()).c_str());
  } else { 
    for(std::set<std::string>::iterator i=datasources.begin();
	i != datasources.end(); ++i){
      QStringList SL(i->c_str());
      SL.append((info + *i).c_str());
      SL.append(rrdfile.c_str());
      SL.append(i->c_str());
      QTreeWidgetItem *it = new QTreeWidgetItem(item, SL);
    }
  }
}

static QTreeWidgetItem *mkItem(QTreeWidget *listview, std::string s)
{
  return new QTreeWidgetItem(listview, QStringList(QString(s.c_str())));
}

static QTreeWidgetItem *mkItem(QTreeWidgetItem *item, std::string s)
{
  return new QTreeWidgetItem(item, QStringList(QString(s.c_str())));
}

void get_rrds(const boost::filesystem::path rrdpath, QTreeWidget *listview)
{
  using namespace boost::filesystem;
  
  const directory_iterator end_itr;
  for (directory_iterator host(rrdpath); host != end_itr; ++host ) {
    if (is_directory(*host)) {
      QTreeWidgetItem *hostitem = mkItem(listview, host->leaf());
      hostitem->setFlags(hostitem->flags() & ~Qt::ItemIsSelectable);
      for (directory_iterator sensor(*host); sensor != end_itr; ++sensor ) {
	if (is_directory(*sensor)) {
	  QTreeWidgetItem *sensoritem = mkItem(hostitem, sensor->leaf());
	  sensoritem->setFlags(hostitem->flags() & ~Qt::ItemIsSelectable);
	  for (directory_iterator rrd(*sensor); rrd != end_itr; ++rrd ) {
	    if (is_regular(*rrd) && extension(*rrd) == ".rrd") {
	      QTreeWidgetItem *rrditem = mkItem(sensoritem, basename(*rrd));
	      rrditem->setFlags(hostitem->flags() & ~Qt::ItemIsSelectable);
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
  // QT4
  #define VERSION "0.1"
  KCmdLineArgs::init(argc, argv, "kcollectd", "",
	ki18n("KCollectd"), VERSION, 
	ki18n("Viewer for Collectd-databases"));
  KApplication a;
  KCollectdGui gui;
  
  try {
    get_rrds(RRD_BASEDIR, gui.listview);
  } 
  catch(basic_filesystem_error<path> &e) {
    KMessageBox::error(0, i18n("Failed to read collectd-structure at \'%1\'\n"
		"Terminating.", QString(RRD_BASEDIR)));
    exit(1);
  } 
  catch(bad_rrdinfo &e) {
    KMessageBox::error(0, e.what());
    exit(2);
  }
  
  a.setTopWidget(&gui);
  gui.show();
  return a.exec();
}
