all: rombios
rombios:
	gcc -E rombios.c > _rombios_.c
	/usr/lib/bcc/bcc-cc1 -o rombios.s -c -D__i86__ -0 _rombios_.c
	sed -e 's/^\.section/\/\/.section/' -e 's/^\.text//' -e 's/^\.data//' rombios.s > _rombios_.s
	as86 _rombios_.s -b rombios.bin -u- -w- -g -0 -j -O -l rombios.txt
