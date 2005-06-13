all: clean readcd

clean:
	rm -f readcd

readcd:
	gcc -DDEBUG_ISO -DSTANDALONE -I./ -I ../../fs/cdrom -o readcd main.c ../../fs/cdrom/iso9660.c
