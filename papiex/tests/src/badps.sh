#!/bin/bash
shell=`basename \`ps -p $$ -ocomm=\``
echo $shell
