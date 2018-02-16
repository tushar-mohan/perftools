#!/bin/bash

# This script will convert one or more profiles generated using hpcrun-flat to JSON. 
# The script is limited in the following ways:
# - It can only be used on profile measuring ONE event
# - The profiles be either for unthreaded programs, or must 
#   aggregate across threads (-tall option for hpcrun-flat)

# 
# usage:
# hpcrun2json <hpcrun-profile> ...

VERSION="1.1.0"
usage="
	$(basename $0) <profile> [profile]...

	The profile(s) must be generated using hpcrun-flat with
	exactly one event, and all thread profiles must be aggregated (-tall).
	e.g.,
	    $ mpirun -np 16 hpcrun-flat -e PAPI_TOT_CYC:99999 -tall ./mpi_wave
	    $ $(basename $0) *.hpcrun-flat.*

"


case $1 in
    *help|-h|--h) echo "$usage" >&2; exit 0;;
    -V|--version) echo "$VERSION" ; exit 0;;
esac

tmpfile=$(mktemp)
hpcproftt  --src=sum $* | sed '/Program/{n;N;d} ; s/^[ \t]*//; s/[ \t]*$//; s/^[-=]*$//g; s/%//g;  s/\s\+/ /g; /^$/d' > "$tmpfile"

events=$(grep ev/smpl $tmpfile| awk '{print $2}'| cut -f1 -d.| uniq)
nevents=$(echo -e "$events"|wc -l)

# FIXME: this limitation of allowing a single event has got to go
if [ $nevents -gt 1 ]; then
    echo "$(basename $0) at present can only handle single event sampling with hpcrun" >&2
    exit 1
fi

echo "Events ($nevents): $events" >&2
ncols=$(grep ev/smpl $tmpfile|wc -l)
nranks=$(echo $ncols/$nevents | bc)
echo "Ranks: $nranks" >&2

total="$(sed -n '/^Program/{n;p}' $tmpfile)"

################################
# awk script                   #
################################
read -d '' awkScript << 'EOF'
BEGIN {
    split(t, total,/ /)
}
{
    for (i=0; i<NF-1; i++) { 
        data[$NF][i] = $(i+1)
    }
}
END{
    for (i in data) { 
        for (rank in data[i]) {
            # the total array index starts from 1, while rank starts from 0
            value = data[i][rank] * total[rank+1]
            if (value > 0) {
                print "  {\\"collector\\": \\"hpcrun-flat\\", \\"type\\": \\"data\\", \\"scope\\": \\""scope"\\", \\"event\\": ", 0", \\"rank\\":", rank, ", \\"value\\": ", value, ", \\"key\\": \\""i"\\"},"
            }
        }
    }
}
EOF
################################
# End of awk Script            #
################################

echo '{"precs": ['

# FIXME: we use $events as if it has only a single event below

(   echo '  {"collector": "hpcrun-flat", "type": "metadata", "events": [{"name": "'$events'"}], "scopes": ["lm", "file", "proc"], "version": "'$VERSION'"}, ';
    sed -n '/^Load/,/^File/p' $tmpfile | sed '1d;$d' | awk -v t="$total" -v scope=lm "$awkScript" ;
    sed -n '/^File/,/^Procedure/p' $tmpfile | sed '1d;$d' | awk -v t="$total" -v scope=file "$awkScript" ;
    sed -n '/^Procedure/,/^Loop/p' $tmpfile | sed '1d;$d' | awk -v t="$total" -v scope=proc "$awkScript" ) | sed '$ s/,$//'

echo "]}"
rm -f $tmpfile
