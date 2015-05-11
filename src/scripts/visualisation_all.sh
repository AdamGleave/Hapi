#!/bin/bash

TO_COMPILE="app/netgen_.*
app/goto_.*
app/quincy_.*
com/.*
inc/.*
opt/.*"

echo -n $TO_COMPILE | parallel --delimiter " " -j 4 $HOME/project/src/tests/py/visualisation.py {}
