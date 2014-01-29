# -*- coding: utf-8 -*-

#-------------------------------------------------------------------------------

# This file is part of Code_Saturne, a general-purpose CFD tool.
#
# Copyright (C) 1998-2014 EDF S.A.
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
# Street, Fifth Floor, Boston, MA 02110-1301, USA.

#-------------------------------------------------------------------------------

"""
Parse command line arguments and launch the GUI.
"""

#-------------------------------------------------------------------------------
# Library modules import
#-------------------------------------------------------------------------------

import os, sys, string

from optparse import OptionParser

if not hasattr(sys, 'version_info') or sys.version_info <= (2, 4, 0, 'final'):
    raise SystemExit("Graphical user interface requires python 2.4 or later.")

#-------------------------------------------------------------------------------
# Third-party modules
#-------------------------------------------------------------------------------

if sys.version_info[0] == 2:
    import sip
    sip.setapi('QString', 2)
    sip.setapi('QVariant', 2)
    sip.setapi('QTime', 2)
    sip.setapi('QUrl', 2)

try:
    from PyQt4.QtCore import *
    from PyQt4.QtGui  import *
except ImportError:
    print("\n  Error: Unable to import PyQt4.QtCore or PyQt4.QtGui modules.")
    print("  Please check your PyQt4 installation.\n")
    sys.exit(0)


if list(map(int, QT_VERSION_STR.split( "."))) < [4, 3, 0]:
    raise SystemExit("Graphical user interface requires Qt 4.3 or later "\
                     "(found %s)." % QT_VERSION_STR)


if list(map(int, PYQT_VERSION_STR.split("."))) < [4, 3, 0]:
    raise SystemExit("Graphical user interface requires PyQt 4.3 or later "\
                     "(found %s)." % PYQT_VERSION_STR)

#-------------------------------------------------------------------------------
# Application modules
#-------------------------------------------------------------------------------

import cs_config

#-------------------------------------------------------------------------------
# Processes the passed command line arguments
#-------------------------------------------------------------------------------

def process_cmd_line(argv):
    """
    Processes the passed command line arguments.
    """

    if sys.argv[0][-3:] == '.py':
        usage = "usage: %prog [options]"
    else:
        usage = "usage: %prog gui [options]"

    parser = OptionParser(usage=usage)

    parser.add_option("-p", "--param", dest="file_name", type="string",
                      metavar="<file>",
                      help="upload a previous case at the interface start")

    parser.add_option("-n", "--new", dest="new",
                      action="store_true",
                      help="open a new case")

    parser.add_option("-z", "--no-splash", dest="splash_screen",
                      action="store_false",
                      help="deactivate splash screen")


    parser.set_defaults(splash_screen=True)

    (options, args) = parser.parse_args(argv)


    if options.new and options.file_name:
        parser.error("Options --new and --param are mutually exclusive")

    if options.new:
        options.file_name = "new case"

    if len(args) > 0:
        if options.file_name or len(args) > 1:
            parser.error("Multiple filenames are given")
        else:
            options.file_name = args[0]

    return options.file_name, options.splash_screen

#-------------------------------------------------------------------------------
# Main
#-------------------------------------------------------------------------------

def main(argv, pkg):
    """
    Start Qt and a session of the application.
    """

    from cs_exec_environment import set_modules, source_rcfile
    set_modules(pkg)
    source_rcfile(pkg)

    # Test the package name to know which modules have to be imported
    if pkg.name == 'code_saturne':
        icons_path = os.path.join(pkg.get_dir('pkgpythondir'), 'Base', 'icons')
        sys.path.insert(1, os.path.join(pkg.get_dir('pkgpythondir'), 'Base'))
    else:
        icons_path = os.path.join(pkg.get_dir('pkgpythondir'), 'core', 'icons')
        sys.path.insert(1, os.path.join(pkg.get_dir('pkgpythondir'), 'core'))

    # Test if EOS modules could be imported
    cfg = cs_config.config()
    if cfg.libs['eos'].have == "yes":
        eosprefix = cfg.libs['eos'].prefix
        try:
            from distutils import sysconfig
            eospath = os.path.join(sysconfig.get_python_lib(0, 0, prefix=eosprefix), 'eos')
        except Exception:
            eospath = ''

        if sys.platform.startswith('win'):
            eospath = os.path.join(eosprefix,
                          'lib', 'python' + sys.version[:3], 'site-packages',
                          'eos')

        if eospath:
            if os.path.isdir(eospath) and not eospath in sys.path:
                sys.path.insert(0, eospath)

    case, spl = process_cmd_line(argv)

    app = QApplication(argv)
    app.setOrganizationName(pkg.code_name) # Defines the name of subdirectory under .config
    app.setOrganizationDomain(pkg.url)
    app.setApplicationName("gui") # Defines the name of the configuration file
    #app.setWindowIcon(QIcon(":/icon.png"))
    app.connect(app, SIGNAL("lastWindowClosed()"), app, SLOT("quit()"))

    # Locale detection
    locale = QLocale.system().name()
    translator = QTranslator(app)
    if translator.load("qt_" + locale,
                       QLibraryInfo.location(QLibraryInfo.TranslationsPath)):
        app.installTranslator(translator)

    if spl:
        app.setOverrideCursor(QCursor(Qt.WaitCursor))
        pixmap = QPixmap('%s/splashscreen.png' % icons_path)
        splash = QSplashScreen(pixmap, Qt.WindowStaysOnTopHint)
        splash.setMask(pixmap.mask()) # this is usefull if the splashscreen is not a regular ractangle...
        splash.show()
        if pkg.name == 'neptune_cfd':
            splash.showMessage("%(name)s %(vers)s starting..." \
                               % {'name': pkg.name, 'vers':pkg.version},
                               Qt.AlignHCenter | Qt.AlignVCenter, Qt.black)
        app.processEvents()
        QTimer.singleShot(1500, splash.hide)

    from Base.MainView import MainView
    mv = MainView(cmd_package = pkg, cmd_case = case)

    try:
        mv.show()
        if spl:
            app.processEvents()
            app.restoreOverrideCursor()
    except:
        print("\n  Unable to display a Qt window.")
        print("  Please check your display environment.\n")
        sys.exit(0)

    sys.exit(app.exec_())

#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
