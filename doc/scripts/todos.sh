#!/bin/bash

FILES=/home/adam/project/doc/diss*.tex

if [[ $# -gt 0 ]]; then
  FILES=$@
fi

grep -n -E "TODO|TBC|XXX|SOMEDAY|FIGURE|\\todo" $FILES
