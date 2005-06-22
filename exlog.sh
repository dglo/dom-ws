#!/bin/bash

#
# add projects of interest here...
#
projects=(dom-ws hal configboot iceboot dom-loader stf dor-test dor-driver tinydaq)
#projects=(dom-ws)

#
# add proper CVSROOT here...
#
export CVSROOT=:ext:arthur@glacier.lbl.gov:/home/icecube/cvsroot

if [[ "$#" == "2" ]]; then
    year=$1
    month=$2
elif [[ "$#" != "0" ]]; then
    echo "usage: exlog.sh [year month]"
    exit 1
else
    year=`date +%Y`
    month=`date +%m`
fi

rm -rf /tmp/exlog
mkdir /tmp/exlog

touch /tmp/exlog/all.days

#
# collect data...
#
for project in ${projects[*]}; do
    (cd ..; cvs -q log $project | \
	gawk -v year=${year} -v month=${month} -v project=${project} \
	    -f dom-ws/exlog.awk)
done

nentries=`wc -l /tmp/exlog/all.entries | awk '{ print $1; }'`

if [[ ${nentries} -gt 0 ]]; then
    sort -n /tmp/exlog/all.entries | sort -n +0 > /tmp/exlog/cooked.entries
    mv /tmp/exlog/cooked.entries /tmp/exlog/all.entries

    #
    # make our report...
    #
    prtDate=`date --date "${year}-${month}-01" '+%B, %Y'`
    echo "CVS commit summary for ${prtDate}"
    projs=`echo ${projects[*]} | sed 's/ /, /g'`
    echo "  projects: ${projs}" 
    users=(`awk '{print $5;}' /tmp/exlog/all.entries | sort | uniq`)
    prtUsers=`echo ${users[*]} | sed 's/ /, /g'`
    echo "  users: ${prtUsers}"
    
    #
    # first, get a list of days...
    #
    days=(`awk -F "\t" '{ print $7; }' /tmp/exlog/all.entries |\
	sort | uniq`)

    for day in ${days[*]}; do
	echo ""
        date --date "${year}-${month}-${day}" '+*** %A, %B %d, %Y ***'
	echo ""

	#
	# now we get each transaction for this day...
	#
	transactions=(`awk -v day=${day} -F "\t" \
	    '{ if ( $7 == day) print $1; }' /tmp/exlog/all.entries | \
	    uniq`)

	for transaction in ${transactions[*]}; do
	    #
	    # get author
	    #
	    author=`grep "^${transaction}	" /tmp/exlog/all.entries |\
		awk -F "\t" '{print $5;}' | head -1`

	    linesModified=`grep "^${transaction}	" \
		/tmp/exlog/all.entries | \
		awk -F "\t" '{print $6; }' | \
		awk '{ sum = sum + ($1 - $2); } END { print sum; }'`

	    if [[ ${lines} == "1" ]]; then
		line="line"
	    else
		line="lines"
	    fi
	    echo "  ${author}, ${linesModified} ${line} modified"

	    #
	    # get projects and packages touched, one line per
	    # project...
	    #
	    tprojects=`grep "^${transaction}	" /tmp/exlog/all.entries | \
		awk -F "\t" '{ print $2;}' | sort | uniq`

	    for tproject in ${tprojects}; do

	        tpackages=(`grep "^${transaction}	${tproject}	" \
		    /tmp/exlog/all.entries | awk -F "\t" '{print $3; }' | \
		    sort | uniq`)
		plist=`echo ${tpackages[*]} | sed 's/ /, /g'`

		tfiles=(`grep "^${transaction}	${tproject}	" \
		    /tmp/exlog/all.entries | awk -F "\t" '{print $4;}' | \
		    sort | uniq`)
		    
		if [[ ${#tfiles[*]} -gt 3 ]]; then
		    flistt=(${tfiles[0]} ${tfiles[1]} ${tfiles[2]} \
			$(( ${#tfiles[*]} - 3 )) )
		    flist=`echo ${flistt[*]} | sed 's/ /, /g'`
		    flist=`echo ${flist} "more..."`
		else
		    flist=`echo ${tfiles[*]} | sed 's/ /, /g'`
		fi

		echo "    ${tproject} [${plist}]: ${flist}"
		
	    done
	    awk '{ print "      " $0; }' /tmp/exlog/${transaction}.msg
	    echo ""
	done
    done
else
    prtDate=`date --date "${year}-${month}-01" '+%B, %Y'`
    echo "Nothing to report for ${prtDate}"
    rm -rf /tmp/exlog
    exit 0
fi

#
# report totals...
#
echo "-------"
echo "Totals:"

function clen() {
    if [[ ${#1} -gt 6 ]]; then
	echo $(( ${#1} + 1 ))
    else
	echo 7
    fi
}

printf "%-9s" "User"
for project in ${projects[*]}; do
    cl=`clen ${project}`
    printf "%${cl}s" ${project}
done

printf "%7s\n" Totals

#
# one line for each user...
#
for user in ${users[*]}; do
    printf "%-9s" ${user}
    
    #
    # totals for each project...
    #
    for project in ${projects[*]}; do
        cl=`clen ${project}`
	printf "%${cl}d" \
	    `grep "^.*	${project}	.*	.*	${user}	" \
		/tmp/exlog/all.entries | \
		awk -F "\t" '{ print $6; }' | \
		awk '{ sum = sum + ($1 - $2); } END { print sum; }'`
    done

    printf "%7d\n" \
	`grep "^.*	.*	.*	.*	${user}	" \
	    /tmp/exlog/all.entries | \
	    awk -F "\t" '{ print $6; }' | \
	    awk '{ sum = sum + ($1 - $2); } END { print sum; }'`
done

rm -rf /tmp/exlog










