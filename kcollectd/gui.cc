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
#include <set>
#include <sstream>
#include <stack>

#include <boost/filesystem.hpp>

#include <QAction>
#include <QDomDocument>
#include <QDrag>
#include <QFile>
#include <QFileDialog>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QSplitter>
#include <QStandardPaths>
#include <QTreeWidget>
#include <QUrl>
#include <QWhatsThis>
#include <QWidget>
#include <QXmlStreamWriter>

#include <KIconLoader>
#include <KLocalizedString>
#include <KMainWindow>
#include <KStandardAction>
#include <KToggleAction>
#include <kactioncollection.h>
#include <kmessagebox.h>

#include "graph.h"
#include "gui.h"
#include "rrd_interface.h"

#include "drag_pixmap.xpm"

#include "../config.h"

#define I18N_NOOP(text) text

static struct {
  KStandardAction::StandardAction actionType;
  const char *name;
  const char *slot;
} standard_actions[] = {
    {KStandardAction::ZoomIn, "zoomIn", SLOT(zoomIn())},
    {KStandardAction::ZoomOut, "zoomOut", SLOT(zoomOut())},
    {KStandardAction::Open, "open", SLOT(load())},
    {KStandardAction::SaveAs, "save", SLOT(save())},
    {KStandardAction::Quit, "quit", SLOT(close())},
};

static struct {
  const char *label;
  const char *name;
  const char *slot;
} normal_actions[] = {
    {I18N_NOOP("Last Hour"), "lastHour", SLOT(last_hour())},
    {I18N_NOOP("Last Day"), "lastDay", SLOT(last_day())},
    {I18N_NOOP("Last Week"), "lastWeek", SLOT(last_week())},
    {I18N_NOOP("Last Month"), "lastMonth", SLOT(last_month())},
    {I18N_NOOP("Add New Subgraph"), "splitGraph", SLOT(splitGraph())},
};

static const std::string delimiter("•");

static void get_datasources(const std::string &rrdfile, const std::string &info,
                            QTreeWidgetItem *item) {
  std::set<std::string> datasources;
  get_dsinfo(rrdfile, datasources);

  if (datasources.size() == 1) {
    item->setFlags(item->flags() | Qt::ItemIsSelectable);
    item->setText(1, QString::fromUtf8(info.c_str()));
    item->setText(2, QString::fromUtf8(rrdfile.c_str()));
    item->setText(3, QString::fromUtf8((*datasources.begin()).c_str()));
  } else {
    for (std::set<std::string>::iterator i = datasources.begin();
         i != datasources.end(); ++i) {
      QStringList SL(i->c_str());
      SL.append(QString::fromUtf8((info + delimiter + *i).c_str()));
      SL.append(QString::fromUtf8(rrdfile.c_str()));
      SL.append(QString::fromUtf8(i->c_str()));
      QTreeWidgetItem *dsitem = new QTreeWidgetItem(item, SL);
      item->setFlags(dsitem->flags() & ~Qt::ItemIsSelectable);
    }
  }
}

static QTreeWidgetItem *mkItem(QTreeWidget *listview, std::string s) {
  return new QTreeWidgetItem(listview, QStringList(QString(s.c_str())));
}

static QTreeWidgetItem *mkItem(QTreeWidgetItem *item, std::string s) {
  return new QTreeWidgetItem(item, QStringList(QString(s.c_str())));
}

static void recurseTree(QTreeWidgetItem *item,
                        const boost::filesystem::directory_iterator &host) {
  const boost::filesystem::directory_iterator end_itr;
  const QString separator = QStringLiteral("-");

  for (int childNumber = 0; childNumber < item->childCount(); ++childNumber) {
    QTreeWidgetItem *newroot = item->child(childNumber);
    QString rootstr = newroot->text(0);
    int lastpos = rootstr.lastIndexOf(separator);

    while (childNumber + 1 < item->childCount()) {
      QTreeWidgetItem *newchild = item->child(childNumber + 1);
      QString newchildstr = newchild->text(0);
      int endsearch = std::min(lastpos + 1, newchildstr.length());
      bool abort = true;
      int newlastpos = -1;

      for (int characterIndex = 0; characterIndex < endsearch;
           ++characterIndex) {
        const QChar character = newchildstr.at(characterIndex);
        if (character != rootstr.at(characterIndex))
          break;

        if (character == separator) {
          newlastpos = characterIndex;
          if (abort) {
            if (newroot->childCount() == 0)
              new QTreeWidgetItem(newroot, QStringList(rootstr));
            item->removeChild(newchild);
            newroot->addChild(newchild);
            abort = false;
          }
        }
      }

      if (abort)
        break;

      lastpos = newlastpos;
    }

    if (newroot->childCount() > 0) {
      newroot->setText(0, rootstr.left(lastpos));
      for (int newchildNumber = 0; newchildNumber < newroot->childCount();
           ++newchildNumber) {
        QTreeWidgetItem *newchild = newroot->child(newchildNumber);
        newchild->setText(0, newchild->text(0).mid(lastpos + 1, -1));
        newchild->setFlags(newchild->flags() & ~Qt::ItemIsSelectable);
      }

      // continue building subfolders
      recurseTree(newroot, host);
      continue;
    }

    // this is the terminal node corresponding to
    // the full directory
    std::stack<std::string> stack;
    std::ostringstream sensorbuilder;

    for (const QTreeWidgetItem *current = newroot; current != NULL;
         current = current->parent())
      stack.push(current->text(0).toStdString());

    // don't use the hostname
    stack.pop();

    // don't start with a separator symbol
    sensorbuilder << stack.top();
    stack.pop();

    for (; stack.size() > 0; stack.pop())
      sensorbuilder << separator.toStdString() << stack.top();

    std::string sensor = sensorbuilder.str();

    for (boost::filesystem::directory_iterator rrd(absolute(*host) / sensor);
         rrd != end_itr; ++rrd) {
      if (!is_regular(*rrd) && extension(*rrd) == ".rrd")
        continue;

      QTreeWidgetItem *rrditem = mkItem(newroot, basename(*rrd));
      rrditem->setFlags(rrditem->flags() & ~Qt::ItemIsSelectable);

      std::ostringstream info;
      info << sensor << delimiter << basename(*rrd);

      get_datasources(rrd->path().string(), info.str(), rrditem);
    }
  }
}

static void get_rrds(const boost::filesystem::path rrdpath,
                     QTreeWidget *listview) {
  using namespace boost::filesystem;

  const directory_iterator end_itr;
  for (directory_iterator host(rrdpath); host != end_itr; ++host) {
    if (!is_directory(*host))
      continue;

    QTreeWidgetItem *hostitem =
        mkItem(listview, host->path().filename().string());
    hostitem->setFlags(hostitem->flags() & ~Qt::ItemIsSelectable);

    for (directory_iterator sensor(*host); sensor != end_itr; ++sensor) {
      if (!is_directory(*sensor))
        continue;

      QTreeWidgetItem *sensoritem =
          mkItem(hostitem, sensor->path().filename().string());
      sensoritem->setFlags(sensoritem->flags() & ~Qt::ItemIsSelectable);
    }

    hostitem->sortChildren(0, Qt::AscendingOrder);
    recurseTree(hostitem, host);
  }

  listview->sortItems(0, Qt::AscendingOrder);
}

/**
 * Constructs a KCollectdGui
 *
 * @param parent parent-widget see KMainWindow
 */
KCollectdGui::KCollectdGui(QWidget *parent)
    : KMainWindow(parent), rrdbasedir(), mHelpMenu(this), action_collection(parent) {
  // standard_actions
  for (size_t i = 0; i < sizeof(standard_actions) / sizeof(*standard_actions);
       ++i)
    actionCollection()->addAction(standard_actions[i].actionType,
                                  standard_actions[i].name, this,
                                  standard_actions[i].slot);
  // normal actions
  for (size_t i = 0; i < sizeof(normal_actions) / sizeof(*normal_actions);
       ++i) {
    QAction *act = new QAction(i18n(normal_actions[i].label), this);
    connect(act, SIGNAL(triggered()), this, normal_actions[i].slot);
    actionCollection()->addAction(normal_actions[i].name, act);
  }
  // toggle_actions
  auto_action = new QAction(QIcon::fromTheme("chronometer"),
                            i18n("Automatic Update"), this);
  auto_action->setCheckable(true);
  auto_action->setShortcut(QKeySequence("f8"));
  actionCollection()->addAction("autoUpdate", auto_action);
  connect(auto_action, SIGNAL(toggled(bool)), this, SLOT(autoUpdate(bool)));

  panel_action = new QAction(i18n("Hide Sensor Data Tree"), this);
  panel_action->setCheckable(true);
  panel_action->setShortcut(QKeySequence("f9"));
  actionCollection()->addAction("hideTree", panel_action);
  connect(panel_action, SIGNAL(toggled(bool)), this, SLOT(hideTree(bool)));

  // build widgets
  QWidget *main_widget = new QWidget(this);
  setCentralWidget(main_widget);

  QHBoxLayout *hboxLayout = new QHBoxLayout(main_widget);
  treeSplitter_ = new QSplitter;
  hboxLayout->addWidget(treeSplitter_);
  listview_ = new QTreeWidget;
  listview_->setColumnCount(1);
  listview_->setHeaderLabels(QStringList(i18n("Sensor Data")));
  listview_->setRootIsDecorated(true);
  listview_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
  treeSplitter_->addWidget(listview_);

  QWidget *vboxWidget = new QWidget;
  vbox = new QVBoxLayout;
  vboxWidget->setLayout(vbox);
  treeSplitter_->addWidget(vboxWidget);
  graph = new Graph;
  vbox->addWidget(graph);
  connect(treeSplitter_, SIGNAL(splitterMoved(int, int)), this, SLOT(resizeTree(int, int)));

  QHBoxLayout *hbox2 = new QHBoxLayout;
  vbox->addLayout(hbox2);
  QPushButton *last_month = new QPushButton(i18n("last month"));
  hbox2->addWidget(last_month);
  QPushButton *last_week = new QPushButton(i18n("last week"));
  hbox2->addWidget(last_week);
  QPushButton *last_day = new QPushButton(i18n("last day"));
  hbox2->addWidget(last_day);
  QPushButton *last_hour = new QPushButton(i18n("last hour"));
  hbox2->addWidget(last_hour);
  QPushButton *zoom_in =
      new QPushButton(QIcon::fromTheme("zoom-in"), QString());
  zoom_in->setToolTip(i18n("increases magnification"));
  zoom_in->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  hbox2->addWidget(zoom_in);
  QPushButton *zoom_out =
      new QPushButton(QIcon::fromTheme("zoom-out"), QString());
  zoom_out->setToolTip(i18n("reduces magnification"));
  zoom_out->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  hbox2->addWidget(zoom_out);
  auto_button = new QPushButton(QIcon::fromTheme("chronometer"), QString());
  auto_button->setToolTip(i18n("toggle automatic update-and-follow mode."));
#if 1
  QString text =
      i18n("<p>This button toggles the "
           "automatic update-and-follow mode</p>"
           "<p>The automatic update-and-follow mode updates the graph "
           "every ten seconds. "
           "In this mode the graph still can be zoomed, but always displays "
           "<i>now</i> near the right edge and "
           "can not be scrolled any more.<br />"
           "This makes kcollectd some kind of status monitor.</p>");
  auto_button->setWhatsThis(text);
#endif
  auto_button->setCheckable(true);
  auto_button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  hbox2->addWidget(auto_button);

  // signals
  connect(listview_, SIGNAL(itemPressed(QTreeWidgetItem *, int)),
          SLOT(startDrag(QTreeWidgetItem *, int)));
  connect(last_month, SIGNAL(clicked()), this, SLOT(last_month()));
  connect(last_week, SIGNAL(clicked()), this, SLOT(last_week()));
  connect(last_day, SIGNAL(clicked()), this, SLOT(last_day()));
  connect(last_hour, SIGNAL(clicked()), this, SLOT(last_hour()));
  connect(zoom_in, SIGNAL(clicked()), this, SLOT(zoomIn()));
  connect(zoom_out, SIGNAL(clicked()), this, SLOT(zoomOut()));
  connect(auto_button, SIGNAL(toggled(bool)), this, SLOT(autoUpdate(bool)));

  // Menu
  QMenu *fileMenu = new QMenu(i18n("&File"));
  menuBar()->addMenu(fileMenu);
  fileMenu->addAction(actionCollection()->action("open"));
  fileMenu->addAction(actionCollection()->action("save"));
  fileMenu->addSeparator();
  fileMenu->addAction(actionCollection()->action("quit"));

  QMenu *editMenu = new QMenu(i18n("&Edit"));
  menuBar()->addMenu(editMenu);
  editMenu->addAction(actionCollection()->action("splitGraph"));

  QMenu *viewMenu = new QMenu(i18n("&View"));
  menuBar()->addMenu(viewMenu);
  viewMenu->addAction(actionCollection()->action("zoomIn"));
  viewMenu->addAction(actionCollection()->action("zoomOut"));
  viewMenu->addSeparator();
  viewMenu->addAction(actionCollection()->action("lastHour"));
  viewMenu->addAction(actionCollection()->action("lastDay"));
  viewMenu->addAction(actionCollection()->action("lastWeek"));
  viewMenu->addAction(actionCollection()->action("lastMonth"));
  viewMenu->addSeparator();
  viewMenu->addAction(actionCollection()->action("autoUpdate"));
  viewMenu->addSeparator();
  viewMenu->addAction(actionCollection()->action("hideTree"));

  menuBar()->addMenu(mHelpMenu.menu());
}

KCollectdGui::~KCollectdGui() {}

void KCollectdGui::startDrag(QTreeWidgetItem *widget, int /*col*/) {
  //       if (event->button() == Qt::LeftButton
  // && iconLabel->geometry().contains(event->pos())) {

  QDrag *drag = new QDrag(this);
  GraphMimeData *mimeData = new GraphMimeData;

  if (widget->text(1).isEmpty())
    return;

  mimeData->setText(widget->text(1));
  mimeData->setGraph(widget->text(2), widget->text(3), widget->text(1));

  drag->setMimeData(mimeData);
  drag->setPixmap(QPixmap(drag_pixmap_xpm));
  drag->exec();
}

void KCollectdGui::set(Graph *new_graph) {
  if (graph) {
    vbox->removeWidget(graph);
    delete graph;
  }
  vbox->insertWidget(0, new_graph);
}

void KCollectdGui::setRRDBaseDir(const QString &newrrdbasedir) {
  if (rrdbasedir.isEmpty()) {
    rrdbasedir = QString(newrrdbasedir);
    // build rrd-tree
    get_rrds(rrdbasedir.toStdString(), listview());
  } 
  // XXX: silently fail on new unimplemented change of rrd base
}

void KCollectdGui::autoUpdate(bool t) {
  auto_button->setChecked(t);
  auto_action->setChecked(t);
  graph->autoUpdate(t);
}

void KCollectdGui::hideTree(bool t) {
  if (t) {
    treeSplitter_->setSizes({0,width()});
  } else {
    int tree_width = listview_->minimumSizeHint().rwidth();
    treeSplitter_->setSizes({tree_width, width()-tree_width});
  }
}

void KCollectdGui::resizeTree(int pos, int) {
  if (pos) {
    panel_action->setChecked(false);
  } else {
    panel_action->setChecked(true);
  }
}

void KCollectdGui::load() {
  QString file = QFileDialog::getOpenFileName(this, "", QDir::homePath(),
                                              "application/x-kcollectd");
  if (file.isEmpty())
    return;

  load(file);
}

void KCollectdGui::save() {
  QString file = QFileDialog::getSaveFileName(this, "", QDir::homePath(),
                                              "application/x-kcollectd");
  if (file.isEmpty())
    return;

  QFile out(file);
  if (out.exists()) {
    int answer =
        KMessageBox::questionYesNo(this, i18n("file ‘%1’ already exists.\n"
                                              "Do you want to overwrite it?",
                                              file));
    if (answer != KMessageBox::Yes) {
      return;
    }
  }
  out.close();
  save(file);
}

void KCollectdGui::load(const QString &file) {
  QFile in(file);
  if (in.open(QIODevice::ReadOnly)) {
    QDomDocument doc;
    doc.setContent(&in);

    graph->clear();

    QDomElement t = doc.documentElement().firstChildElement("tab");
    while (!t.isNull()) {
      QDomElement g = t.firstChildElement("graph");
      while (!g.isNull()) {
        GraphInfo &graphinfo = graph->add();
        QDomElement p = g.firstChildElement("plot");
        while (!p.isNull()) {
          graphinfo.add(p.attribute("rrd"), p.attribute("ds"),
                        p.attribute("label"));
          p = p.nextSiblingElement();
        }
        g = g.nextSiblingElement();
      }
      t = t.nextSiblingElement();
    }
    filename = file;
    graph->changed(false);
  } else {
    KMessageBox::detailedSorry(
        this, i18n("reading file ‘%1’ failed.", filename),
        i18n("System message is: ‘%1’", in.errorString()));
  }
}

void KCollectdGui::save(const QString &file) {
  QFile out(file);
  if (out.open(QIODevice::WriteOnly)) {
    QXmlStreamWriter stream(&out);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();
    stream.writeDTD("<!DOCTYPE kcollectd SYSTEM "
                    "\"/usr/share/kcollectd/kcollectd.xsd\">");

    stream.writeStartElement("kcollectd");
    stream.writeStartElement("tab");
    for (Graph::const_iterator i = graph->begin(); i != graph->end(); ++i) {
      stream.writeStartElement("graph");
      for (GraphInfo::const_iterator j = i->begin(); j != i->end(); ++j) {
        stream.writeStartElement("plot");
        stream.writeAttribute("rrd", j->rrd);
        stream.writeAttribute("ds", j->ds);
        stream.writeAttribute("label", j->label);
        stream.writeEndElement();
      }
      stream.writeEndElement();
    }
    stream.writeEndElement();
    stream.writeEndElement();

    stream.writeEndDocument();
    filename = file;
    graph->changed(false);
  } else {
    KMessageBox::detailedSorry(
        this, i18n("opening the file ‘%1’ for writing failed.", filename),
        i18n("System message is: ‘%1’", out.errorString()));
  }
}

void KCollectdGui::saveProperties(KConfigGroup &conf) {
  conf.writeEntry("hide-navigation", panel_action->isChecked());
  conf.writeEntry("auto-update", graph->autoUpdate());
  conf.writeEntry("range", qint64(graph->range()));
  if (!graph->changed() && !filename.isEmpty()) {
    conf.writeEntry("filename", QDir().absoluteFilePath(filename));
    conf.writeEntry("file-is-session", false);
  } else {
    char hostname[100];
    gethostname(hostname, sizeof(hostname));
    QString file = QString("%1/session-%2-%3")
                       .arg(QStandardPaths::writableLocation(
                           QStandardPaths::RuntimeLocation))
                       .arg(hostname)
                       .arg(getpid());
    save(file);
    conf.writeEntry("filename", file);
    conf.writeEntry("file-is-session", true);
  }
}

void KCollectdGui::readProperties(const KConfigGroup &conf) {
  bool nav = conf.readEntry("hide-navigation", false);
  bool aut = conf.readEntry("auto-update", false);
  time_t range = conf.readEntry("range", 24 * 3600);
  QString file = conf.readEntry("filename", QString());
  bool file_is_session = conf.readEntry("file-is-session", false);
  if (!file.isEmpty()) {
    load(file);
    if (file_is_session) {
      QFile::remove(file);
      graph->changed(true);
    } else {
      filename = file;
    }
  }
  panel_action->setChecked(nav);
  hideTree(nav);
  autoUpdate(aut);
  graph->last(range);
}
