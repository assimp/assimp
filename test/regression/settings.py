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

"""Shared settings for the regression suite (bold builder and
test scripts rely on this)

"""

import os

# -------------------------------------------------------------------------------
# Files to ignore (with reason)
#
# pond.0.ply - loads with 40k identical triangles, causing postprocessing
# to have quadratic runtime.
# -------------------------------------------------------------------------------
files_to_ignore = ["pond.0.ply"]

# -------------------------------------------------------------------------------
# List of file extensions to be excluded from the regression suite
# File extensions are case insensitive
# -------------------------------------------------------------------------------
exclude_extensions = [
    ".assbin", ".assxml", ".txt", ".md",
    ".jpeg", ".jpg", ".png", ".gif", ".tga", ".bmp",
    ".skeleton", ".skeleton.xml", ".license", ".mtl", ".material", ".pk3"
]

# -------------------------------------------------------------------------------
# Post processing configurations to be included in the test. The
# strings are parameters for assimp_cmd, see assimp_cmd's doxydoc
# for more details. 

# The defaults are (validate-data-structure is always enabled, for
# self-explanatory reasons :-):
#
# '-cfull'    :apply all post processing except 'og' and 'ptv' (optimize-scenegraph)
# '-og -om'   :run optimize-scenegraph in combination with optimize-meshes.
# '-vds -jiv' :join-identical-vertices alone. This is a hotspot where
#              floating-point inaccuracies can cause severe damage.
# '-ptv':      transform all meshes to world-space

# As you can see, not all possible combinations of pp steps are covered - 
# but at least each step is executed at least once on each model. 
# -------------------------------------------------------------------------------
pp_configs_to_test = [
    "-cfull",
    "-og -om -vds",
    "-vds -jiv",
    "-ptv -gsn -cts -db",

    # this is especially important: if no failures are present with this 
    # preset, the regression is most likely caused by the post
    # processing pipeline.
    ""
]
# -------------------------------------------------------------------------------
# Name of the regression database file to be used
# gen_db.py writes to this directory, run.py checks against this directory.
# If a zip file with the same name exists, its contents are favoured to a 
# normal directory, so in order to test against unzipped files the ZIP needs
# to be deleted.
# -------------------------------------------------------------------------------
database_name = "db"

# -------------------------------------------------------------------------------
# List of directories to be processed. Paths are processed recursively.
# -------------------------------------------------------------------------------
model_directories = [
    os.path.join("..","models"),
    os.path.join("..","models-nonbsd")
]

# -------------------------------------------------------------------------------
# Remove the original database files after the ZIP has been built?
# -------------------------------------------------------------------------------
remove_old = True

# -------------------------------------------------------------------------------
# Bytes to skip at the beginning of a dump. This skips the file header, which
# is currently the same 500 bytes header for both assbin, assxml and minidumps.
# -------------------------------------------------------------------------------
dump_header_skip = 500

# -------------------------------------------------------------------------------
# Directory to write all results and logs to. The dumps pertaining to failed
# tests are written to a subfolder of this directory ('tmp').
# -------------------------------------------------------------------------------
results = os.path.join("..","results")

# Create results directory if it does not exist
if not os.path.exists(results):
    os.makedirs(results)

# vim: ai ts=4 sts=4 et sw=4
