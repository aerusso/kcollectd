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

#include <exception>
#include <iostream>
#include <string>

#include <boost/filesystem.hpp>

#include <QApplication>
#include <QCommandLineParser>
#include <QStringList>
#include <QTreeWidgetItem>

#include <KAboutData>
#include <KLocalizedString>
#include <KMessageBox>

#include "../config.h"

#include "gui.h"

int main(int argc, char **argv) {
  using namespace boost::filesystem;

  std::vector<std::string> rrds;
  QApplication application(argc, argv);
  KAboutData about(
      "kcollectd", i18n("KCollectd"), VERSION,
      i18n("Viewer for Collectd-databases"), KAboutLicense::GPL_V3,
      i18n("© 2008, 2009 M G Berberich"), i18n("Maintainer and developer"),
      "https://www.antonioerusso.com/projects/kcollectd",
      "Antonio E Russo <antonio.e.russo@gmail.com>");
  about.addAuthor(i18n("M G Berberich"), i18n("Maintainer and developer"),
                  "M G Berberich <berberic@fmi.uni-passau.de>",
                  "http://www.forwiss.uni-passau.de/~berberic");
  about.addAuthor(i18n("A E Russo"), i18n("Maintainer and developer"),
                  "Antonio E Russo <antonio.e.russo@gmail.com>",
                  "https://www.antonioerusso.com");
  about.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"),
                      i18nc("EMAIL OF TRANSLATORS", "Your emails"));

  KAboutData::setApplicationData(about);

  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addVersionOption();

  parser.addPositionalArgument("+[file]", i18n("A kcollectd-file to open"));
  parser.process(application);

  const QStringList args = parser.positionalArguments();
  try {
    if (application.isSessionRestored()) {
      kRestoreMainWindows<KCollectdGui>();
    } else {
      KCollectdGui *gui = new KCollectdGui;
      // handling arguments
      if (args.length() == 1)
        gui->load(args.at(0));
      gui->setObjectName("kcollectd#");
      gui->show();
    }
  } catch (const std::exception &e) {
    KMessageBox::error(0, i18n("Failed to read collectd-structure at \'%1\'\n"
                               "Terminating.",
                               QString(RRD_BASEDIR)));
    exit(1);
  }

  return application.exec();
}
