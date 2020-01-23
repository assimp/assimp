#!/usr/bin/env python3
# -*- Coding: UTF-8 -*-

# ---------------------------------------------------------------------------
# Open Asset Import Library (ASSIMP)
# ---------------------------------------------------------------------------
#
# Copyright (c) 2006-2020, ASSIMP Development Team
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

"""Shared stuff for the gen_db and run scripts"""

# -------------------------------------------------------------------------------
def hashing(file,pp):
    """ Map an input file and a postprocessing config to an unique hash.

    The hash is used to store the item in the database. It 
    needs to be persistent across different python implementations
    and platforms, so we implement the hashing manually.
    """
    file = file.lower()
    file = file.replace('\\','/')+":"+pp
    # SDBM hash
    res = 0
    for t in file:
        res = (ord(t) + (res<<6) + (res<<16) - res) % 2**32
    return '{:x}'.format(res)


 # vim: ai ts=4 sts=4 et sw=4
