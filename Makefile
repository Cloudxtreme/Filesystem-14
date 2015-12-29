all:
	gcc filesystem.c -o filesystem -D_FILE_OFFSET_BITS=64 -Wall `pkg-config fuse --cflags --libs` -std=gnu99 -w
mount:
	./filesystem MyFilesystem/
umount:
	fusermount -u MyFilesystem
mountnew:
	./filesystem MyFilesystem/ newfs
