#!/bin/sh

set -x

# abort on any errors
set -e

# upload to sourceforge
rsync -avP -e ssh Platinum*.* c0diq@frs.sourceforge.net:uploads/
