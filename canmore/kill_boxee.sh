#!/bin/ash
kill -9 `/bin/ps | grep Boxee | /bin/grep -v grep | /usr/bin/awk '{ print $1 }'`
