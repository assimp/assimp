#!/usr/bin/env python3
# -*- Coding: UTF-8 -*-

# ---------------------------------------------------------------------------
# Open Asset Import Library (ASSIMP)
# ---------------------------------------------------------------------------
#
# Copyright (c) 2006-2016, ASSIMP Development Team
#
# All rights reserved.
#
# Redistribution and use of this software in source and binary forms, 
# with or without modification, are permitted provided that the following 
# conditions are met:
# 
# * Redistributions of source code must retain the above
#   copyright notice, this list of conditions and the
#   following disclaimer.
# 
# * Redistributions in binary form must reproduce the above
#   copyright notice, this list of conditions and the
#   following disclaimer in the documentation and/or other
#   materials provided with the distribution.
# 
# * Neither the name of the ASSIMP team, nor the names of its
#   contributors may be used to endorse or promote products
#   derived from this software without specific prior
#   written permission of the ASSIMP Development Team.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# ---------------------------------------------------------------------------
from tkinter import *
import sys
import os
import platform
import run
import subprocess
import result_checker as rc

INFO  = 0
WARN  = 1
ERROR = 2

# -------------------------------------------------------------------------------
def log( sev, msg ):
    """
    This function is used to log info, warnings and errors.
    """
    logEntry = ""
    if sev == 0:
        logEntry = logEntry + "[INFO]: "
    elif sev == 1:
        logEntry = logEntry + "[WARN]: "
    elif sev == 2:
        logEntry = logEntry + "[ERR] : "
    logEntry = logEntry + msg
    print( logEntry )

# -------------------------------------------------------------------------------
class BaseDialog( Toplevel ):
    """
    Helper base class for dialogs used in the UI.
    """

    def __init__(self, parent, title = None, buttons=""):
        """
        Constructor
        """
        Toplevel.__init__( self, parent )
        self.transient(parent)

        if title:
            self.title(title)

        self.parent = parent
        self.result = None
        body = Frame(self)
        self.initial_focus = self.body(body)
        body.pack(padx=5, pady=5)
        self.buttonbox(buttons)
        self.grab_set()
        if not self.initial_focus:
            self.initial_focus = self
        self.protocol("WM_DELETE_WINDOW", self.cancel)
        self.geometry("+%d+%d" % (parent.winfo_rootx() + 50,
                                  parent.winfo_rooty() + 50))
        self.initial_focus.focus_set()
        self.wait_window(self)

    def body(self, master):
        # create dialog body.  return widget that should have
        # initial focus.  this method should be overridden
        pass

    def buttonbox(self, buttons):
        # add standard button box. override if you don't want the
        # standard buttons
        box = Frame(self)
        w = Button(box, text="OK", width=40, command=self.ok, default=ACTIVE)
        w.pack(side=LEFT, padx=5, pady=5)
        self.bind("<Return>", self.ok)
        box.pack()

    def ok(self, event=None):
        if not self.validate():
            self.initial_focus.focus_set()  # put focus back
            return

        self.withdraw()
        self.update_idletasks()
        self.apply()
        self.cancel()

    def cancel(self, event=None):
        # put focus back to the parent window
        self.parent.focus_set()
        self.destroy()

    def validate(self):
        return 1  # override

    def apply(self):
        pass  # override

# -------------------------------------------------------------------------------
class VersionDialog( BaseDialog ):
    """
    This class is used to create the info dialog.
    """
    def body(self, master):
        # info will be read from assimp command line tool
        version = "Asset importer lib version unknown"
        exe = run.getEnvVar( "assimp_path" )
        if len( exe ) != 0:
            command = [exe, "version" ]
            log( INFO, "command = " + str(command))
            stdout = subprocess.check_output(command)
            for line in stdout.splitlines():
                pos = str(line).find( "Version" )
                if -1 != pos:
                    version = line
        Label(master, text=version).pack()

    def apply(self):
        pass

# -------------------------------------------------------------------------------
class SetupDialog( BaseDialog ):
    """
    This class is used to create the setup dialog.
    """
    def body(self, master):
        Label(master, justify=LEFT, text="Assimp: " ).grid(row=0, column=0)
        Label(master, justify=LEFT, text=run.getEnvVar("assimp_path")).grid(row=0, column=1)
        Label(master, text="New executable:").grid(row=1)
        self.e1 = Entry(master)
        self.e1.grid(row=1, column=1)
        return self.e1  # initial focus

    def apply(self):
        exe = str( self.e1.get() )
        if len( exe )  == 0:
            return 0
        if os.path.isfile( exe ):
            log( INFO, "Set executable at " + exe)
            self.assimp_bin_path = exe
            run.setEnvVar("assimp_path", self.assimp_bin_path)
        else:
            log( ERROR, "Executable not found at "+exe )
        return 0

# -------------------------------------------------------------------------------
class RegDialog( object ):
    """
    This class is used to create a simplified user interface for running the regression test suite.
    """
    
    def __init__(self, bin_path ):
        """
        Constructs the dialog, you can define which executable shal be used.
        @param  bin_path    [in] Path to assimp binary.
        """
        run.setEnvVar( "assimp_path", bin_path )
        self.b_run_ = None
        self.b_update_ = None
        self.b_res_checker_ = None
        self.b_quit_ = None
        if platform.system() == "Windows":
            self.editor = "notepad"
        elif platform.system() == "Linux":
            self.editor = "vim"
        self.root = None
        self.width=40
        
    def run_reg(self):
        log(INFO, "Starting regression test suite.")
        run.run_test()
        rc.run()
        self.b_update_.config( state=ACTIVE  )
        return 0

    def reg_update(self):
        assimp_exe = run.getEnvVar( "assimp_path" )
        if len( assimp_exe ) == 0:
            return 1
        exe = "python"
        command = [ exe, "gen_db.py", assimp_exe ]
        log(INFO, "command = " + str(command))
        stdout = subprocess.call(command)

        log(INFO, stdout)
        return 0
    
    def shop_diff( self ):
        log(WARN, "ToDo!")
        return 0
        
    def open_log(self):
        command = [ self.editor, "../results/run_regression_suite_output.txt", ]
        log(INFO, "command = " + str( command ) )
        r = subprocess.call(command)
        return 0

    def show_version( self ):
        d = VersionDialog( self.root )
        return 0
    
    def setup(self):
        d = SetupDialog( self.root )
        return 0

    def quit(self):
        log( INFO, "quit" )
        sys.exit( 0 )

    def initUi(self):
        # create the frame with buttons
        self.root = Tk()
        self.root.title( "Assimp-Regression UI")
        self.b_run_       = Button( self.root, text="Run regression ", command=self.run_reg,    width = self.width )
        self.b_update_    = Button( self.root, text="Update database", command=self.reg_update, width = self.width )
        self.b_show_diff_ = Button( self.root, text="Show diff", command=self.shop_diff, width = self.width )
        self.b_log_       = Button( self.root, text="Open log", command=self.open_log, width = self.width )
        self.b_setup_     = Button( self.root, text="Setup", command=self.setup, width = self.width )
        self.b_version_   = Button( self.root, text="Show version", command=self.show_version, width = self.width )
        self.b_quit_      = Button( self.root, text="Quit", command=self.quit,       width = self.width )
        
        # define the used grid
        self.b_run_.grid(       row=0, column=0, sticky=W+E )
        self.b_update_.grid(    row=1, column=0, sticky=W+E )
        self.b_show_diff_.grid( row=2, column=0, sticky=W+E )
        self.b_log_.grid(       row=3, column=0, sticky=W+E )
        self.b_setup_.grid(     row=4, column=0, sticky=W+E )
        self.b_version_.grid(   row=5, column=0, sticky=W+E )
        self.b_quit_.grid(      row=6, column=0, sticky=W+E )
        
        #self.b_update_.config( state=DISABLED )
        self.b_show_diff_.config( state=DISABLED )
        
        # run mainloop
        self.root.mainloop()

# -------------------------------------------------------------------------------
def getDefaultExecutable():
    assimp_bin_path = ""
    if platform.system() == "Windows":
        assimp_bin_path = '..\\..\\bin\\debug\\assimpd.exe'
    elif platform.system() == "Linux":
        assimp_bin_path = '../../bin/assimp'

    return assimp_bin_path

# -------------------------------------------------------------------------------
if __name__ == "__main__":
    if len(sys.argv) > 1:
        assimp_bin_path = sys.argv[1]
    else:
        assimp_bin_path = getDefaultExecutable()
    log( INFO, 'Using assimp binary: ' + assimp_bin_path )
    dlg = RegDialog(assimp_bin_path)
    dlg.initUi()
   
# vim: ai ts=4 sts=4 et sw=4
