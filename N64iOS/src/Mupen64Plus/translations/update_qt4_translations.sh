#!/bin/sh

DIR=../main
EXCLUDE=gui_gtk
TARGETS=

for t in $(ls ${DIR} -lI ${EXCLUDE} | awk '{ print $8 }')
do
    TARGETS="${DIR}/${t} ${TARGETS}"
done

lupdate ${TARGETS} -ts mupen64plus*.ts

