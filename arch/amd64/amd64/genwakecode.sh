#!/bin/sh
# $NetBSD: genwakecode.sh,v 1.2 2007/12/09 20:32:18 jmcneill Exp $

objdump -t acpi_wakecode.o > acpi_wakecode.bin.map
ld -z defs -nostdlib --oformat binary -e wakeup_16 -Ttext 0 -o acpi_wakecode.bin acpi_wakecode.o


P='/previous_/ { printf("#define\t%s%s\t0x%s\n", $5, length($5) < 16 ? "\t" : "", $1); }'
awk "$P" < acpi_wakecode.bin.map

P='/wakeup_/ { printf("#define\t%s%s\t0x%s\n", $5, length($5) < 16 ? "\t" : "", $1); }'
awk "$P" < acpi_wakecode.bin.map

P='/physical_/ { printf("#define\t%s%s\t0x%s\n", $5, length($5) < 16 ? "\t" : "", $1); }'
awk "$P" < acpi_wakecode.bin.map

P='/where_to_recover/ { printf("#define\t%s%s\t0x%s\n", $5, length($5) < 16 ? "\t" : "", $1); }'
awk "$P" < acpi_wakecode.bin.map

echo 
echo 'static const unsigned char wakecode[] = {';
hexdump -v -e '"\t" 8/1 "0x%02x, " "\n"' < acpi_wakecode.bin | sed 's/0x  /0x00/g'
echo '};'

exit 0
