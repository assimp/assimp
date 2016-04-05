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
from Tkinter import *
import sys
import run
import result_checker as rc

class RegDialog( object ):
    def __init__(self, bin_path ):
        self.assimp_bin_path = bin_path
        self.b_run_ = None
        self.b_update_ = None
        self.b_res_checker_ = None
        self.b_quit_ = None
        
        
    def run_reg(self):
        print "run_reg"
        run.setEnvVar( "assimp_path", self.assimp_bin_path )
        run.run_test()
        rc.run()
        self.b_update_.config( state=ACTIVE  )
        return 0

    def reg_update(self):
        print "ToDo!"
        return 0
    
    def quit(self):
        print "quit"
        sys.exit( 0 )

    def initUi(self):
        root = Tk()
        root.title( "Assimp-Regression")
        self.b_run_ = Button( root, text="Run regression ", command=self.run_reg,    width = 50 )
        self.b_update_ = Button( root, text="Update database", command=self.reg_update, width = 50 )
        self.b_quit_ = Button( root, text="Quit           ", command=self.quit,       width = 50 )
        self.b_run_.grid( row = 0, column = 0, sticky = W+E)
        self.b_update_.grid( row = 1, column = 0, sticky = W+E)
        self.b_quit_.grid( row = 2, column = 0, sticky = W+E)
        self.b_run_.pack()
        self.b_update_.pack()
        self.b_quit_.pack()
        self.b_update_.config( state=DISABLED )
        root.mainloop()

if __name__ == "__main__":
    if len(sys.argv) > 1:
        assimp_bin_path = sys.argv[1]
    else:
        assimp_bin_path = '..\\..\\bin\\debug\\assimpd.exe'
    print( 'Using assimp binary: ' + assimp_bin_path )
    dlg = RegDialog(assimp_bin_path)
    dlg.initUi()
   
# vim: ai ts=4 sts=4 et sw=4