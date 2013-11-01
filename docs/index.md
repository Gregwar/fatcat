# fatcat tutorials & examples

Here is a documentation that will inform you about FAT filesystems,
and explain how you can use the `fatcat` tool to forensic, repair,
undelete and hack FAT:

* [FAT repair guide](repair.md)
* [Use fatcat with a partitionned disk](partition.md)
* [Undelete tutorial: how to revive deleted files](undelete.md)
* [FAT table tutorial: how to fix damaged table](fat.md)
* [Orphan tutorial: how to recover lost files and directories](orphan.md)
* [Fun with FAT: fun filesystem hackings](fun-with-fat.md)

## Images

You can find prebuilt images in `images/` directory:

* `empty.img`: an empty FAT32 filesystem
* `hello-world.img`: a simple image with txt files and a directory
* `deleted.img`: an image containing a directory and a file that was both deleted
* `directory-loop.img`: an image with looping directories
* `infinite-file.img`: a file which is looping and with maximum FAT32 size
  (4G the image is just 50M)
* `full-fat.img`: an image with a full FAT, the disk appear full even
  if you can't see any file in it
* `two-file-same-cluster.img`: an image with two files having different
  names pointing to the same cluster. If you change one, the other will be
  changed too (note that your OS may cache some data).
* `fake-big-disk-1T.img`: an image with fake values in the FAT32 headers,
  so that your system may behaves like you have a 1T disk, even if it's smaller.
  You can read & write files on it until you'll reach the actual size of your
  disk.
* `repair.img`: an image that you can repair to test the fatcat options. it contains
  a directory that is unallocated in FAT1 (can be merged with FAT2 using -m), a directory that
  is unallocated (can be fixed with -f), and an orphan directory (can be found using -o,
  see [orphaned tutorial](orphan.md)). Have a look to the [repair guide](repair.md).

