#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#-------------------------------------------------------------------------------

# This file is part of code_saturne, a general-purpose CFD tool.
#
# Copyright (C) 1998-2023 EDF S.A.
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

import fnmatch
import os
import sys
import tempfile

from optparse import OptionParser

from cs_exec_environment import separate_args, enquote_arg
from cs_compile import cs_compile

#-------------------------------------------------------------------------------

def process_cmd_line(argv):
    """
    Processes the passed command line arguments for a build environment.

    Input Argument:
      arg -- This can be either a list of arguments as in
             sys.argv[1:] or a string that is similar to the one
             passed on the command line.  If it is a string,
             it is split to create a list of arguments.
    """

    parser = OptionParser(usage="usage: %prog [options]")

    parser.add_option("--mode", dest="mode", type="string",
                      metavar="<mode>",
                      help="'build' or 'install'")

    parser.add_option("-d", "--dest", dest="dest_dir", type="string",
                      metavar="<dest_dir>",
                      help="choose executable file directory")

    parser.add_option("-o", "--outfile", dest="out_file", type="string",
                      metavar="<out_file>",
                      help="choose executable file name")

    parser.add_option("--lib-flags-only", dest="lib_flags_only",
                      action="store_true",
                      help="print library link flags only")

    parser.set_defaults(mode='build')
    parser.set_defaults(dest_dir=None)
    parser.set_defaults(out_file=None)
    parser.set_defaults(lib_flags_only=False)

    (options, args) = parser.parse_args(argv)

    return options, args

#-------------------------------------------------------------------------------

def dest_subdir(destdir, d):

    t = d

    # Concatenate destdir and target subdirectory

    if sys.platform.startswith("win"):
        i = t.find(':\\')
        if i > -1:
            t = t[i+1:]
            while t[0] == '\\':
                t = t[1:]
    else:
        while t[0] == '/':
            t = t[1:]

    return os.path.join(destdir, t)

#-------------------------------------------------------------------------------

def src_include_dirs(srcdir):
    """
    Return include directories in a given source directory.
    """
    include_dirs = []

    if srcdir:
        for f in os.listdir(os.path.join(srcdir, 'src')):
            p = os.path.join(srcdir, 'src', f)
            if os.path.isdir(p):
                include_dirs.append(p)

    return include_dirs

#===============================================================================
# Class used to manage compilation in build directory
#===============================================================================

class compile_build(cs_compile):

    def __init__(self,
                 package=None,
                 srcdir=None):
        """
        Initialize compiler object.
        """
        cs_compile.__init__(self, package)
        self.srcdir = srcdir

        top_builddir = os.getcwd()
        while not os.path.isfile(os.path.join(top_builddir, "cs_config.h")):
            ds = os.path.split(top_builddir)
            if ds[1]:
                top_builddir = ds[0]
            else:
                break

        if not os.path.isdir(os.path.join(top_builddir, "src")):
            raise Exception("top build directory not detected from: " \
                            + os.getcwd())

        self.top_builddir = top_builddir

    #---------------------------------------------------------------------------

    def get_compiler(self, compiler):
        """
        Determine the compiler path for a given compiler type.
        """

        return self.pkg.config.compilers[compiler]

    #---------------------------------------------------------------------------

    def flags_relocation(self, flag, cmd_line):

        return

    #---------------------------------------------------------------------------

    def get_pkg_path_flags(self, flag, base_name=None):
        """
        Determine compilation flags for a given flag type.
        """

        flags = []

        top_builddir = self.top_builddir

        # Add CPPFLAGS and LDFLAGS information for the current package
        if flag == 'cppflags':
            if self.pkg.config.libs['ple'].variant == 'internal':
                flags.append('-I' + os.path.join(top_builddir, 'libple'))
                flags.append('-I' + os.path.join(self.srcdir, 'libple', 'src'))
            flags.append('-I' + top_builddir)
            include_dirs = src_include_dirs(self.srcdir)
            for d in include_dirs:
                flags.append('-I' + d)

        elif flag == 'ldflags':
            tsd = os.path.join(top_builddir, 'src')
            for f in os.listdir(tsd):
                p = os.path.join(tsd, f)
                if os.path.isdir(p):
                    flags.append('-L' + p)
            if self.pkg.config.libs['ple'].variant == 'internal':
                flags.append('-L' + os.path.join(top_builddir, 'libple', 'src'))
            l_cwd = '-L' + os.path.join(os.getcwd())
            if not l_cwd in flags:
                flags.append(l_cwd)
            # Add library paths which may be indirectly required
            re_lib_dirs = self.pkg.config.get_compile_dependency_paths()
            for d in re_lib_dirs:
                flags.insert(0, '-L' + d)

        return flags

    #---------------------------------------------------------------------------

    def get_ar_lib_dir(self):
        """
        Determine directory containing library in archive mode.
        """

        return os.path.join(self.top_builddir, "src", "apps")

#===============================================================================
# Class used to manage install
#===============================================================================

class compile_install(cs_compile):

    def __init__(self,
                 package=None,
                 srcdir=None,
                 destdir=None):
        """
        Initialize compiler object.
        """
        cs_compile.__init__(self, package)
        self.srcdir = srcdir

        self.destdir = destdir

    #---------------------------------------------------------------------------

    def get_pkg_path_flags(self, flag, base_name=None):
        """
        Determine compilation flags for a given flag type.
        """

        flags = []

        # Add CPPFLAGS and LDFLAGS information for the current package
        if flag == 'cppflags':
            dirs = []
            dirs.insert(0, self.pkg.dirs['pkgincludedir'])
            if self.pkg.config.libs['ple'].variant == "internal":
                dirs.insert(0, self.pkg.dirs['includedir'])
            for d in dirs:
                if self.destdir:
                    flags.append("-I" + dest_subdir(self.destdir, d))
                else:
                    flags.append("-I" + d)

        elif flag == 'ldflags':
            # Do not use pkg.get_dir here as possible relocation paths must be
            # used after installation, not before.
            libdir = pkg.dirs['libdir']
            # Strangely, on MinGW, Windows paths are not correctly handled here
            # So, assuming we always build on MinGW, here is a little trick!
            if sys.platform.startswith("win"):
                if pkg.get_cross_compile() != 'cygwin': # mingw64
                    libdir = os.path.normpath('C:\\MinGW\\msys\\1.0' + libdir)
            if self.destdir:
                libdir = dest_subdir(self.destdir, libdir)
            flags.append("-L" + libdir)

        return flags

    #---------------------------------------------------------------------------

    def get_compiler(self, compiler):
        """
        Determine the compiler path for a given compiler type.
        """

        return self.pkg.config.compilers[compiler]

    #---------------------------------------------------------------------------

    def flags_relocation(self, flag, cmd_line):

        return

    #---------------------------------------------------------------------------

    def get_flags(self, flag):
        """
        Determine compilation flags for a given flag type.
        """

        if flag == 'libs':
            return cs_compile.get_flags(self, flag)

        cmd_line = self.get_pkg_path_flags(flag)

        # Build the command line, and split possible multiple arguments in lists.
        for lib in pkg.config.deplibs:
            if (pkg.config.libs[lib].have == True \
                and (not pkg.config.libs[lib].dynamic_load)):
                cmd_line += separate_args(pkg.config.libs[lib].flags[flag])

        if flag == 'ldflags':
            # Do not use pkg.get_dir here as possible relocation paths
            # must be used after installation, not before.
            libdir = pkg.dirs['libdir']
            # Strangely, on MinGW, Windows paths are not correctly
            # handled here. So, assuming we always build on MinGW,
            # here is a little trick!
            if sys.platform.startswith("win"):
                if pkg.get_cross_compile() != 'cygwin': # mingw64
                    libdir = os.path.normpath('C:\\MinGW\\msys\\1.0' + libdir)
            if self.destdir:
                libdir = dest_subdir(self.destdir, libdir)
            cmd_line.insert(0, "-L" + libdir)

        return cmd_line

    #---------------------------------------------------------------------------

    def get_lib_dir(self):
        """
        Determine directory containing library.
        """

        cmd_line = self.get_pkg_path_flags(flag)

        # Build the command line, and split possible multiple arguments in lists.
        for lib in pkg.config.deplibs:
            if (pkg.config.libs[lib].have == True \
                and (not pkg.config.libs[lib].dynamic_load)):
                cmd_line += separate_args(pkg.config.libs[lib].flags[flag])

        print(cmd_line)
        print(self.pkg.config.rpath)

#===============================================================================
# Functions
#===============================================================================

def install_exec_name(pkg, exec_name, destdir=None):
    """
    Determine full executable path and create associated directory
    if necessary
    """

    exec_name = os.path.join(pkg.dirs['pkglibexecdir'], exec_name)
    # Strangely, on MinGW, Windows paths are not correctly handled here...
    # So, assuming we always build on MinGW, here is a little trick!
    if sys.platform.startswith("win"):
        if pkg.get_cross_compile() != 'cygwin': # mingw64
            exec_name = os.path.normpath('C:\\MinGW\\msys\\1.0' + exec_name)
        else:
            exec_name = os.path.join(pkg.dirs['pkglibexecdir'],
                                     os.path.basename(exec_name))
    if destdir:
        exec_name = dest_subdir(destdir, exec_name)
    dirname = os.path.dirname(exec_name)
    if not os.path.exists(dirname):
        os.makedirs(dirname)

    return exec_name

#------------------------------------------------------------------------------

def get_dynamic_lib_dep_flags(pkg, top_builddir=None, destdir=None):
    """
    List dynamic library dependency flags.
    """

    cmd_line = ""
    rpath_list = []

    libdir = None
    if not top_builddir:
        libdir = pkg.dirs['libdir']
        if destdir:
            libdir = dest_subdir(destdir, libdir)

    for lib in pkg.config.deplibs:
        if pkg.config.libs[lib].have == False:
            continue
        if pkg.config.libs[lib].dynamic_load:
            continue

        add_rpath = pkg.config.libs[lib].add_rpath
        if lib == 'saturne':
            # For libsaturne.so, no need to add self
            continue
        else:
            ldflags = pkg.config.libs[lib].flags['ldflags']
            if lib == 'ple' and pkg.config.libs['ple'].variant == 'internal':
                if top_builddir:
                    ldflags = "-L" + enquote_arg(os.path.join(top_builddir,
                                                              'libple', 'src'))
                elif libdir:
                    ldflags = "-L" + enquote_arg(os.path.join(libdir))
        if ldflags:
            cmd_line += ldflags + " "
            if add_rpath:
                for o in separate_args(ldflags):
                    if o[:2] == '-L':
                        rpath_list.append(o[2:].strip())

        libs = pkg.config.libs[lib].flags['libs']
        if libs:
            cmd_line += libs + " "

    if rpath_list:
        cmd_line += "-Wl,-rpath -Wl," + ':'.join(rpath_list)

    return cmd_line.rstrip()

#-------------------------------------------------------------------------------

if __name__ == '__main__':

    # Check mode and options
    options, src_files = process_cmd_line(sys.argv[1:])

    top_builddir = os.getenv("CS_TOP_BUILDDIR")
    config_file_base = os.path.join("lib", "code_saturne_build.cfg")

    if top_builddir:
        top_builddir = os.path.abspath(top_builddir)
        config_file = os.path.join(top_builddir, config_file_base)
    else:
        top_builddir = os.path.abspath(os.getcwd())
        t = os.path.split(top_builddir)
        while (t[1]):
            config_file = os.path.join(top_builddir, config_file_base)
            if os.path.isfile(config_file):
                break;
            t = os.path.split(top_builddir)
            top_builddir = os.path.abspath(t[0])

    # Retrieve package information (name, version, installation dirs, ...)

    from cs_package import package

    pkg = package(config_file=config_file, install_mode=True)

    src_dir = None
    if src_files:
        src_dir = os.path.dirname(os.path.dirname(sys.argv[0]))

    if options.lib_flags_only:
        if options.mode == 'install':
            print(get_dynamic_lib_dep_flags(pkg, destdir=options.dest_dir))
        else:
            print(get_dynamic_lib_dep_flags(pkg, top_builddir=top_builddir))
        sys.exit(0)

    # Determine executable name

    exec_name=options.out_file
    if not exec_name:
        exec_name = 'cs_solver'
        if os.path.basename(sys.argv[0]) == 'neptune_cfd':
            exec_name = 'nc_solver'

    if options.mode == 'install':
        c = compile_install(pkg, src_dir, destdir=options.dest_dir)
        exec_name = install_exec_name(pkg, exec_name, options.dest_dir)
    else:
        c = compile_build(pkg, src_dir)

    retcode = 0
    o_files = None
    if src_files:
        retcode, o_files = c.compile_src(src_list=src_files)

    if retcode == 0:
        print("Linking executable: " + exec_name)
        retcode = c.link_obj(exec_name, o_files)

    sys.exit(retcode)

#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
