# !/bin/sh

if [[ $1 == "" ]]; then
    echo "usage: tcal.sh tcalfile"
    exit 1;
fi

gawk -f tcal.awk $1 | gawk -f tcal-calc.awk | dc > /tmp/rt-times.$$
avg=`awk '{ sum += $1; n++; } END { print 1.0*sum/n; }' /tmp/rt-times.$$`
dev=`awk -vavg=${avg} '{ sum +=  ($1 - avg)*($1 - avg); n++; } \
    END { print sqrt(sum/(n-1)); }' /tmp/rt-times.$$`

echo ${avg} ${dev}

rm -f /tmp/rt-times.$$
