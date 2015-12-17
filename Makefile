all:
	gcc filesystem.c -o filesystem -lfuse -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26
mount:
	./filesystem MyFilesystem/
umount:
	fusermount -u MyFilesystem
