#!/bin/sh
#
# $FreeBSD: src/sys/tools/fdt/make_dtbh.sh,v 1.1 2010/06/02 17:22:38 raj Exp $

# Script generates a $2/fdt_static_dtb.h file.

dtb_base_name=`basename $1 .dts`
echo '#define FDT_DTB_FILE "'${dtb_base_name}.dtb'"' > $2/fdt_static_dtb.h
