#!/bin/bash
cat ServiceDescription.h | sed 's/"$//g' | sed 's/^\([\t ]*\)"/\1/g' | sed 's/\\"/"/g' > ServiceMethods.json
