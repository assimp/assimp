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

"""Generate the regression database db.zip from the files in the <root>
/test/models directory. Older databases are overwritten with no prompt.
"""

import sys
import os
import subprocess
import zipfile

import settings
import utils

# -------------------------------------------------------------------------------
def process_dir(d, outfile, file_filter):
    """ Generate small dump records for all files in 'd' """
    print("Processing directory " + d)
    for f in os.listdir(d):
        fullp = os.path.join(d, f)
        if os.path.isdir(fullp) and not f == ".svn":
            process_dir(fullp, outfile, file_filter)
            continue

        if file_filter(f):
            for pp in settings.pp_configs_to_test:
                print("DUMP " + fullp + "\n post-processing: " + pp)
                outf = os.path.join(os.getcwd(), settings.database_name,
                    utils.hashing(fullp, pp))

                cmd = [utils.assimp_bin_path,"dump",fullp,outf,"-b","-s",pp]
                outfile.write("assimp dump "+"-"*80+"\n")
                outfile.flush()
                if subprocess.call(cmd, stdout=outfile, stderr=outfile, shell=False):
                    print("Failure processing " + fullp)
                    

# -------------------------------------------------------------------------------
def make_zip():
    """Zip the contents of ./<settings.database_name>"""
    zipout = zipfile.ZipFile(settings.database_name + ".zip", "w", zipfile.ZIP_DEFLATED)
    for f in os.listdir(settings.database_name):
        p = os.path.join(settings.database_name, f)
        zipout.write(p, f)
        if settings.remove_old:
            os.remove(p)

    if settings.remove_old:
        os.rmdir(settings.database_name)

    bad = zipout.testzip()
    assert bad is None
    

# -------------------------------------------------------------------------------
def gen_db():
    """Generate the crash dump database in ./<settings.database_name>"""
    try:
        os.mkdir(settings.database_name)
    except OSError:
        pass
    
    outfile = open(os.path.join("..", "results", "gen_regression_db_output.txt"), "w")
    (ext_list, err) = subprocess.Popen([utils.assimp_bin_path, "listext"],
        stdout=subprocess.PIPE).communicate()
    
    ext_list = str(ext_list).lower().split(";")
    ext_list = list(filter(
        lambda f: not f in settings.exclude_extensions, map(
            lambda f: f.strip("b* \'"), ext_list)))

    print(ext_list)
    for tp in settings.model_directories:
        process_dir(tp, outfile,
            lambda x: os.path.splitext(x)[1] in ext_list)
        

# -------------------------------------------------------------------------------
if __name__ == "__main__":
    utils.find_assimp_or_die()
    gen_db()
    make_zip()

    print("="*60)
    input("Press any key to continue")
    
# vim: ai ts=4 sts=4 et sw=4    

