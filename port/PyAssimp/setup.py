 # -*- coding: utf-8 -*-
import os
from distutils.core import setup

setup(name='pyassimp',
      version='0.1',
      license='ISC',
      description='Python bindings for the Open Asset Import Library (ASSIMP)',
      url='https://github.com/assimp/assimp',
      packages=['pyassimp'],
      data_files=[('share/pyassimp', ['README.md']),
                  ('share/examples/pyassimp', ['scripts/' + f for f in os.listdir('scripts/')])]
      )
