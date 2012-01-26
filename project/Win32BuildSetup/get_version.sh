expr `git log --pretty=oneline | wc -l` + 15012 | tr -d '\n' > REVISION
echo -n '-' >> REVISION
echo -n `git rev-parse --short HEAD` >> REVISION
cat ../../xbmc/linux/svn_rev.h.in  | sed 's/WCREV/'`cat REVISION`/ > ../../xbmc/linux/svn_rev.h
echo -n `cat ../../xbmc/lib/libBoxee/bxversion.h | grep BOXEE_VERSION | awk '{print $3}' | sed 's/"//g'` > VERSION
echo -n . >> VERSION
cat REVISION >> VERSION