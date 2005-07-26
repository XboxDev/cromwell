EXTRA_CFLAGS := -DNO_DECOMPRESSION -DFSYS_REISERFS -DFSYS_EXT2FS -DFSYS_UFS2 -I$(TOPDIR)/fs/grub -DSTAGE1_5 -DNO_GETENV -DCROMWELL

O_TARGET := fsys_reiserfs.o fsys_ext2fs.o fsys_ufs2.o char_io.o disk_io.o

include $(TOPDIR)/Rules.make
