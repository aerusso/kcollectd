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

#include <iostream>

#include <qlayout.h>
#include <qlabel.h>
#include <qvbox.h>

#include <klistview.h>

#include "graph.h"
#include "gui.moc"

BlaFasel::BlaFasel(QWidget *parent, const char *name)
  : QVBox(parent, name)
{
  QHBox *hbox = new QHBox(this);
  listview = new KListView(hbox);
  listview->addColumn("Sensordata");
  listview->setRootIsDecorated(true);
  listview->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  connect(listview, SIGNAL(executed(QListViewItem *)), 
	SLOT(selectionChanged(QListViewItem *)));

  QWidget *w = new QWidget(hbox);
  w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  QVBoxLayout *vbox = new QVBoxLayout(w);
  graph = new Graph(w);
  graph->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  vbox->addWidget(graph);
  label = new QLabel(w);
  vbox->addWidget(label);  
}

void BlaFasel::selectionChanged(QListViewItem * item)
{
  if (item && item->text(1)) {
    // std::cout << "clicked: " << item->text(0) << "→"
    // 	      << item->text(2) << "\n";
  
    graph->setup(item->text(2), item->text(0));

    label->setText(QString(item->text(1)));

    // std::cout << "BlaFasel::selectionChanged: Calling graph->update ()...\n";
    // std::cout.flush ();

    graph->update();
  }
}
