#!/usr/bin/env python

from setuptools import setup, Extension
import numpy as np
import os

# Extension modules
fstd_core = Extension ('pygeode_rpn.fstd_core', sources=['fstd_core.c'], libraries=['rmn'])

# Check if the librmn static library is available for local compilation.
if os.path.exists(os.path.join('python-rpn-libsrc','librmn','Makefile')):
  from subprocess import check_call
  libdir = os.path.abspath('python-rpn-libsrc')
  check_call(['make', 'PWD='+libdir], cwd=libdir)
  fstd_core.libraries = ['rmn_019.2']
  fstd_core.extra_compile_args.append('-g')
  fstd_core.extra_link_args.extend(['-L'+libdir+'/librmn','-lgfortran'])

# PyGeode-RPN installation script
setup (	name="pygeode-rpn",
	version="2.3.1",
        author="Mike Neish",
        install_requires=['pygeode >= 1.2.0'],
	ext_modules=[fstd_core],
        include_dirs = [np.get_include()],
	packages=["pygeode_rpn"],
        package_dir = {"pygeode_rpn":""},
        entry_points = {
          'pygeode.formats': [
            'fstd = pygeode_rpn.fstd',
            'fstd_core = pygeode_rpn.fstd_core',
          ],
          'pygeode.axis': [
          'atmosphere_hybrid_sigma_ln_pressure_coordinate = pygeode_rpn.fstd:LogHybrid',
          ]
        },
	scripts=["fstd2nc"]
)

