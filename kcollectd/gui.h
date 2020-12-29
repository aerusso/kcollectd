/* -*- c++ -*- */
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

#ifndef GUI_H
#define GUI_H

#include <KConfigGroup>
#include <KMainWindow>
#include <KHelpMenu>
#include <kactioncollection.h>

#include "graph.h"

class QLabel;
class Graph;
class QSplitter;
class QTreeWidget;
class QTreeWidgetItem;
class QVBoxLayout;
class QAction;
class QPushButton;

class KCollectdGui : public KMainWindow // QWidget
{
  Q_OBJECT;

public:
  KCollectdGui(QWidget *parent = 0);
  virtual ~KCollectdGui();

  QTreeWidget *listview() { return listview_; }
  KActionCollection *actionCollection() { return &action_collection; }

  void set(Graph *graph);
  void setRRDBaseDir(const QString &newrrdbasedir);
  void load(const QString &filename);
  void save(const QString &filename);

public slots:
  void startDrag(QTreeWidgetItem *widget, int col);
  virtual void last_month();
  virtual void last_week();
  virtual void last_day();
  virtual void last_hour();
  virtual void zoomIn();
  virtual void zoomOut();
  virtual void autoUpdate(bool active);
  virtual void hideTree(bool active);
  virtual void resizeTree(int pot, int);
  virtual void splitGraph();
  virtual void load();
  virtual void save();

protected:
  virtual void saveProperties(KConfigGroup &) override;
  virtual void readProperties(const KConfigGroup &) override;

private:
  QTreeWidget *listview_;
  QSplitter *treeSplitter_;
  QVBoxLayout *vbox;
  Graph *graph;
  QPushButton *auto_button;
  QAction *auto_action, *panel_action;
  QString filename;
  QString rrdbasedir;
  KHelpMenu mHelpMenu;

  KActionCollection action_collection;
};

inline void KCollectdGui::last_month() { graph->last(3600 * 24 * 31); }

inline void KCollectdGui::last_week() { graph->last(3600 * 24 * 7); }

inline void KCollectdGui::last_day() { graph->last(3600 * 24); }

inline void KCollectdGui::last_hour() { graph->last(3600); }

inline void KCollectdGui::zoomIn() { graph->zoom(1.0); }

inline void KCollectdGui::zoomOut() { graph->zoom(-1.0); }

inline void KCollectdGui::splitGraph() { graph->splitGraph(); }

#endif
