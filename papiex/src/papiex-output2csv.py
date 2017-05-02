#!/usr/bin/env python

# usage:
# papiex-output2csv files...
# papiex-output2csv <dir>
# 
# Honors the following environment variables:
# PAPIEX_CSV_OUTFILE : output file name
# DEBUG

import re
import sys
import os

debug=os.getenv("DEBUG",False)
quiet=False

def input2dict(infile):
    '''Create a dictionary by reading in the file'''
    global quiet
    d = {}
    f = open(infile, 'r')
    for line in f:
        if 'QUIET' in line:
            quiet = True
        if (re.match(r'^Event descriptions:$', line)):
            break;
        if re.match(r'[+-]?\d+\s+.*', line):
            #645593387        Real cycles\n
            l = line.split(' ', 1) # [645593387, '       Real cycles\n']
            v = l[0] # 645593387
            k = l[-1].lstrip().rstrip() # 'Real cycles'
        elif re.match(r'\w.*\s+ : .*', line):
            #Parent process id             : 86
            #Start                         : Thu Nov 19 04:24:41 2015
            l = line.split(' : ', 1) # ['Parent process id    ', 86]
            k = l[0].rstrip()
            v = l[-1].lstrip().rstrip()
        else:
            continue
        k = k.replace(',','_')
        v = v.replace(',','_')
        # skip tuples with empty value fields
        if (v != ''):
            d[k] = v
    f.close()
    return(d)


def dictlist2csv(dlist, outfile = None):
    import csv
    # we need to produce a union of all the keys across all the dicts in dlist
    keys_set = set([])
    for d in dlist:
        keys_set.update(d.keys())
    # keys = sorted(dlist[0].keys())
    keys = sorted(list(keys_set))
    f = open(outfile, 'wb') if outfile else sys.stdout
    dict_writer = csv.DictWriter(f, keys)
    dict_writer.writeheader()
    dict_writer.writerows(dlist)

def get_files_list(rootdir,exclude_filter=''):
    flist = []
    for root, subFolders, files in os.walk(rootdir):
        flist += [ root + os.sep + f for f in files ]
    ret = flist if not(exclude_filter) else ([ f for f in flist if not(exclude_filter) in f ])
    return ret

def get_output_name(args):
    if (len(args) > 1): return ''
    path = args[0]
    if os.path.isdir(path): return (path + '.csv')
    return os.path.splitext(path)[0]+'.csv'
            
if __name__ == "__main__":
    arg = sys.argv[1]
    json_list = []
    file_list = []
    outfile = os.getenv('PAPIEX_CSV_OUTFILE', get_output_name(sys.argv[1:]))

    for arg in sys.argv[1:]:
        file_list += get_files_list(arg, 'report.txt') if os.path.isdir(arg) else [arg]
    for f in file_list:
        if (debug): print "processing %s" % (f)
        json_list.append(input2dict(f))
    if (outfile and not(quiet)):
        if (debug): print "papiex: csv output saved to %s" % (outfile)
    dictlist2csv(json_list, outfile)
