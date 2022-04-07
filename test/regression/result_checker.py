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

"""
This script runs as part of the Travis CI build on Github and controls
whether a patch passes the regression test suite.

Given the failures encountered by the regression suite runner (run.py) in
  ../results/whitelist.csv
and the current whitelist of failures considered acceptable in
  ./run_regression_suite_failures_whitelisted.csv
determine PASSED or FAILED.

"""

import sys
import os

RESULTS_FILE = os.path.join('..', 'results', 'run_regression_suite_failures.csv')
WHITELIST_FILE = os.path.join('whitelist.csv')

BANNER = """
*****************************************************************
Regression suite result checker
(test/regression/result_checker.py)
*****************************************************************
"""

def passed(message):
    print('\n\n**PASSED: {0}.\n'.format(message))
    return 0

def failed(message):
    print('\n\n**FAILED: {0}. \nFor more information see test/regression/README.\n'
        .format(message))
    return -1

def read_results_csv(filename):
    parse = lambda line: map(str.strip, line.split(';')[:2])
    try:
        with open(filename, 'rt') as results:
            return dict(parse(line) for line in results.readlines()[1:])
    except IOError:
        print('Failed to read {0}.'.format(filename))
        return None

def run():
    print(BANNER)
    print('Reading input files.')
    
    result_dict = read_results_csv(RESULTS_FILE)
    whitelist_dict = read_results_csv(WHITELIST_FILE)
    if result_dict is None or whitelist_dict is None:
        return failed('Could not locate input files')

    if not result_dict:
        return passed('No failures encountered')

    print('Failures:\n' + '\n'.join(sorted(result_dict.keys())))
    print('Whitelisted:\n' + '\n'.join(sorted(whitelist_dict.keys())))
    non_whitelisted_failures = set(result_dict.keys()) - set(whitelist_dict.keys())
    print('Failures not whitelisted:\n' + '\n'.join(sorted(non_whitelisted_failures)))
    if not non_whitelisted_failures:
        return passed('All failures are whitelisted and considered acceptable \n' +
            'due to implementation differences, library shortcomings and bugs \n' +
            'that have not been fixed for a long time')
    return failed('Encountered new regression failures that are not whitelisted.  \n' +
        'Please carefully review the changes you made and use the gen_db.py script\n' +
        'to update the regression database for the affected files')

if __name__ == "__main__":
    sys.exit(run())

# vim: ai ts=4 sts=4 et sw=4
