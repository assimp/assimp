#!/usr/bin/env python3
# -*- Coding: UTF-8 -*-

# ---------------------------------------------------------------------------
# Open Asset Import Library (ASSIMP)
# ---------------------------------------------------------------------------
#
# Copyright (c) 2006-2010, ASSIMP Development Team
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

"""Shared stuff for the gen_db and run scripts """

# -------------------------------------------------------------------------------
def hashing(file,pp):
    """ Map an input file and a postprocessing config to an unique hash.

    The hash is used to store the item in the database. It 
    needs to be persistent across different python implementations
    and platforms, so we implement the hashing manually.
    """

    def myhash(instring):
        # sdbm hash
        res = 0
        for t in instring:
            res = (ord(t) + (res<<6) + (res<<16) - res) % 2**32
        return res

    return hex(myhash(file.replace('\\','/')+":"+pp))


assimp_bin_path = None
# -------------------------------------------------------------------------------
def find_assimp_or_die():
    """Find assimp_cmd's binary for the current platform.
    
    The path to the binary is stored in assimp_bin_path, the process
    is aborted if it can't be found.

    """

    import os
    import platform
    import sys

    def locate_file(f_list):
        for f in f_list:
            try:
                fl = open(f,"rb")
            except IOError:
                continue
            fl.close()
            return f
        return None

    global assimp_bin_path
    if os.name == "nt":
        search_x86 = [
            os.path.join("..","..","bin","assimpcmd_release-dll_Win32","assimp.exe"),
            os.path.join("..","..","bin","x86","assimp"),
            os.path.join("..","..","bin","Release","assimp.exe")
        ]
        if platform.machine() == "x86":
            search = search_x86
        else: # amd64, hopefully
            search = [
                os.path.join("..","..","bin","assimpcmd_release-dll_x64","assimp.exe"),
                os.path.join("..","..","bin","x64","assimp")
            ]
            # x64 platform does not guarantee a x64 build. Also look for x86 as last paths.
            search += search_x86
        
        assimp_bin_path = locate_file(search)
        if assimp_bin_path is None:
            print("Can't locate assimp_cmd binary")
            print("Looked in", search)
            sys.exit(-5)

        print("Located assimp/assimp_cmd binary from", assimp_bin_path)
    elif os.name == "posix":
        #search = [os.path.join("..","..","bin","gcc","assimp"),
        #    os.path.join("/usr","local","bin",'assimp')]
        assimp_bin_path = "assimp"
        print("Taking system-wide assimp binary")
    else:
        print("Unsupported operating system")
        sys.exit(-5)

if __name__ == '__main__':
    find_assimp_or_die()

 # vim: ai ts=4 sts=4 et sw=4
