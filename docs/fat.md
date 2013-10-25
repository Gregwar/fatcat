# FAT table tutorial: how to fix damaged table

Back to [documentation index](index.md)

## Dealing with different FAT tables

Sometimes your disk can be damaged, the FAT tables can be broken too.

Let's [create a disk](disk.md) and "break" it as an example:

```
# mount disk.img disk/
# echo "Hello world" > disk/hello.txt
# umount disk.img
# fatcat disk.img -w 2 -v 0 -t 1
Writing next cluster of 2 from 4294967295 to 0
Writing on FAT1
```

With this `fatcat` command, we are writing 0 to the cluster table for the
entry 2 (which is root directory) in the fat table 1.

This will break the filesystem because the root directory will have an
unallocated cluster. Both Windows and Linux will fail mounting this new image.

What's surprising is that there is not only one FAT table, but two. The second
will still contains a good value:

```
# fatcat disk.img -@ 2
Cluster 2 address:
823296 (00000000000c9000)
Next cluster:
FAT1: 0 (00000000)
FAT2: 4294967295 (ffffffff)
Chain size: 2
```

As you can see, there is still the `0xffffffff` value in the FAT2 table.

You can do a full check of differences in the two tables with `-2`:

```
# fatcat disk.img -2
Comparing the FATs
[00000002] 1:00000000 2:ffffffff

FATs differs
It seems mergeable
```

The cluster 2 appears here and the difference is clearly indicated by `fatcat`.

Since 0 is the value for unallocated entries, and is also very likely what will be read
if your disk is damaged, non-zero values in the diff are more relevant than zero.

This is why `fatcat` comes with an option to merge the tables, `-m`:

```
# fatcat disk.img -m
Begining the merge...
Merging cluster 2
Merge complete, 1 clusters merged
```

Doing that, the FAT tables are merged and the disk image is now fixed.

## Backup and restore of FAT tables

In order to do some hacks, you may want sometimes to backup and/or restore your
fat table(s):

```
# fatcat disk.img -b out.fat
Successfully wrote out.fat (806912)
```

This will backup your two FAT tables by default, you can use `-t` to backup just
the table 1 or 2:

```
# fatcat disk.img -b out1.fat -t 1
Successfully wrote out1.fat (403456)
```

In a similar way, you can use `-p` to apply a fat table to your disk:

```
# fatcat disk.img -p out.fat
```

You can use this to erase one of your table for instance:

```
# fatcat disk.img -p /dev/zero -t 1
```

This will erase the table 1 with zeroes.
