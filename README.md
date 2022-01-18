# fatcat

![fatcat](docs/fatcat.jpg)

This tool is designed to manipulate FAT filesystems, in order to
explore, extract, repair, recover and forensic them. It currently
supports FAT12, FAT16 and FAT32.

[Tutorials & examples](docs/index.md)

## Building and installing

You can build `fatcat` this way:

```
mkdir build
cd build
cmake ..
make
```

And then install it:

```
make install
```

## Exploring

### Using fatcat

Fatcat takes an image as argument:

```
fatcat disk.img [options]
```

**NOTE: according to your build, you might have to specify options before ``disk.img``, i.e
``fatcat [options] disk.img``**

You can specify an offset in the file with `-O`, this could be useful if there is 
multiple partitions on a block devices, for instance:

```
fatcat disk.img -O 1048576 [options]
```

This will tell fatcat to begin on the 1048576th byte. Have a look to the [partition tutorial](docs/partition.md).

### Listing

You can explore the FAT partition using `-l` option like this:

```
$ fatcat disk.img -l /
Listing path /
Cluster: 2
d 24/10/2013 12:06:00  some_directory/                c=4661
d 24/10/2013 12:06:02  other_directory/               c=4662
f 24/10/2013 12:06:40  picture.jpg                    c=4672 s=532480 (520K)
f 24/10/2013 12:06:06  hello.txt                      c=4671 s=13 (13B)
```

You can also provide a path like `-l /some/directory`.

Using `-L`, you can provide a cluster number instead of a path, this may
be useful sometime.

If you add `-d`, you will also see deleted files.

In the listing, the prefix is `f` or `d` to tell if the line concerns a file or
a directory.

The `c=` indicates the cluster number, `s=` indicates the site in bytes (which
should be the same as the pretty size just after).

The `h` letter at the end indicates that the file is supposed to be hidden.

The `d` letter at the end indicates that the file was deleted.

### Reading a file

You can read a file using `-r`, the file will be wrote on the standard
output:

```
$ fatcat disk.img -r /hello.txt
Hello world!
$ fatcat disk.img -r /picture.jpg > save.jpg
```

Using `-R`, you can provide a cluster number instead of a path, but the file size
information will be lost and the file will be rounded to the number of clusters
it fits, unless you provide the `-s` option to specify the file size to read.

You can use `-x` to extract the FAT filesystem directories to a directory:

```
fatcat disk.img -x output/
```

If you want to extract from a certain cluster, provide it with `-c`.

If you provide `-d` to extract, deleted files will be extracted too.

## Undelete

### Browsing deleted files & directories

As explaines above, deleted files can be found in listing by providing `-d`:

```
$ fatcat disk.img -l / -d
f 24/10/2013 12:13:24  delete_me.txt                  c=5764 s=16 (16B) d
```

You can explore and spot a file or an interesting deleted directory.

### Retrieving deleted file

To retrieve a deleted file, simply use `-r` to read it. Note that the produced
file will be read contiguously from the original FAT system and may be broken.

### Retreiving deleted directory

To retrieve a deleted directory, note its cluster number and extract it like above:

```
# If your deleted directory cluster is 71829
fatcat disk.img -x output/ -c 71829
```

See also: [undelete tutorial](docs/undelete.md)

## Recover

### Damaged file system

Assuming your disk has broken sectors, you may want to do recovering on it.

The first advice is to make a copy of your data using `ddrescue`, and save your disk
to another one or into a sane file.

When sectors are broken, their bytes will be replaced with `0`s in the `ddrescue` image.

A first way to go is trying to explore your image using `-l` as above and check `-i` to
find out if `fatcat` recognizes the disk as a FAT system.

Then, you can try to have a look at `-2`, to check if the file allocation tables differs,
and if it looks mergeable. It is very likely that is will be mergeable, in this case, you
can try `-m` to merge the FAT tables, don't forget to backup it before (see below).

### Orphan files

When your filesystem is broken, there are files and lost files and lost directories that
we call "orphaned", because you can't reach them from the normal system.

`fatcat` provides you an option to find those nodes, it will do an automated analysis of your
system and explore allocated sectors of your filesystem, this is done with `-o`.

You will get a list of directories and files, like this:

```
There is 2 orphaned elements:
Directory clusters 4592 to 4592: 2 elements, 49B
File clusters 4611 to 4611: ~512B
```

You can then use directly `-L` and `-R` to have a look into those files and directories:

```
$ fatcat disk.img -L 4592
Listing cluster 4592
Cluster: 4592
d 23/10/2013 17:45:06  ./                             c=4592
d 23/10/2013 17:45:06  ../                            c=0
f 23/10/2013 17:45:22  poor_orphan.txt                c=4601 s=49 (49B)
```

Note that orphan files have an unknown size, this mean that if you read it, you will get
a file that is a multiple of the cluster sizes.

See also: [orphaned files tutorial](docs/orphan.md)

## Hacking

You can use `fatcat` to hack your FAT filesystem

### Informations

The `-i` flag will provide you a lot of information about the filesystem:

```
fatcat disk.img -i
```

This will give you headers data like sectors sizes, fats sites, disk label etc. It
will also read the FAT table to estimate the usage of the disk.

You can also get information about a specific cluster by using `-@`:

```
fatcat disk.img -@ 1384
```

This will give you the cluster address (offset of the cluster in the filesystem)
and the value of the next cluster in the two FAT tables.

### Backuping & restoring FAT

You can use `-b` to backup your FAT tables:

```
fatcat disk.img -b backup.fats
```

And use `-p` to write it back:

```
fatcat disk.img -p backup.fats
```

### Writing to the FATs

You can write to the FAT tables with `-w` and `-v`:

```
fatcat disk.img -w 123 -v 124
```

This will write `124` as value of the next cluster of `123`.

You can also choose the table with `-t`, 0 is both tables, 1 is the first and 2 the second.

### Diff & merge the FATs

You can have a look at the diff of the two FATs by using `-2`:

```
# Watching the diff
$ fatcat disk.img -2
Comparing the FATs

FATs are exactly equals

# Writing 123 in the 500th cluster only in FAT1
$ fatcat disk.img -w 500 -v 123 -t 1
Writing next cluster of 500 from 0 to 123
Writing on FAT1

# Watching the diff
$ fatcat disk.img -2
Comparing the FATs
[000001f4] 1:0000007b 2:00000000

FATs differs
It seems mergeable
```

You can merge two FATs using `-m`. For each different entries in the table,
if one is zero and not the other, the non-zero file will be choosen:

```
$ fatcat disk.img -m
Begining the merge...
Merging cluster 500
Merge complete, 1 clusters merged
```

See also: [fixing fat tutorial](docs/fat.md)

### Directories fixing

Fatcat can fix directories having broken FAT chaining.

To do this, use `-f`. All the filesystem tree will be walked and the directories
that are unallocated in the FAT but that fatcat can read will be fixed in the FAT.

### Entries hacking

You can have information about an entry with `-e`:

```
fatcat disk.img -e /hello.txt
```

This will display the address of the entry (not the file itself), the cluster reference
and the file size (if not a directory).

You can add the flag `-c [cluster]` to change the cluster of the entry and the flag
`-s [size]` to change the entry size.

See also: [fun with fat tutorial](docs/fun-with-fat.md)

You can use `-k` to search for a cluster reference.

### Erasing unallocated files

You can erase unallocated sectors data, with zeroes using `-z`, or using
random data using `-S`.

For instance, deleted files will then become unreadables.

## LICENSE

This is under MIT license
