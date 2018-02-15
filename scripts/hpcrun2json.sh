#!/bin/bash
tmpfile=$(mktemp)
hpcproftt -S *.hpcstruct --src=sum *.hpcrun-flat.* | sed '/Program/{n;N;d} ; s/^[ \t]*//; s/[ \t]*$//; s/^[-=]*$//g; s/%//g;  s/\s\+/ /g; /^$/d' > "$tmpfile"

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
# AWK script                   #
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
                print "  {\\"collector\\": \\"hpcrun-flat\\", \\"type\\": \\"data\\", \\"scope\\": \\""scope"\\", \\"event\\": ", 0", \\"rank\\":", rank, ", \\"value\\": ", value, ", \\"key\\": \\""i"\\"}, "
            }
        }
    }
}
EOF
################################
# End of AWK Script            #
################################

echo "["

# FIXME: we use $events as if it has only a single event below
echo '  {"collector": "hpcrun-flat", "type": "metadata", "events": [{"name": "'$events'"}], "scopes": ["lm", "file", "proc"]}, '


sed -n '/^Load/,/^File/p' $tmpfile | sed '1d;$d' | awk -v t="$total" -v scope=lm "$awkScript"
sed -n '/^File/,/^Procedure/p' $tmpfile | sed '1d;$d' | awk -v t="$total" -v scope=file "$awkScript"
sed -n '/^Procedure/,/^Loop/p' $tmpfile | sed '1d;$d' | awk -v t="$total" -v scope=proc "$awkScript"

echo "]"
rm -f $tmpfile
