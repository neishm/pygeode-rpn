#!/usr/bin/env python

from setuptools import setup, Extension
import numpy as np

# Extension modules
fstd_core = Extension ('pygeode_rpn.fstd_core', sources=['fstd_core.c'], libraries=['rmn'])

# PyGeode installation script

setup (	name="pygeode-rpn",
	version="2.2.0",
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

