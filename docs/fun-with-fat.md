# Fun with FAT: fun filesystem hackings

Back to [documentation index](index.md)

Note: have a look at the [prebuilt images](index.md)

## Tutorials

### The directory loop

In FAT32, directories are just a list of entries which can point to files or
other directories.

Other directories are referenced using their cluster. With `fatcat`, you can
use the `-L` flag to list a directory using its cluster number instead of
its path, for instance:

```
fatcat disk.img -L 2
```

This will list the directory that is in the cluster 2. In most of FAT filesystems,
this will be the root directory (you can check that out using `-i`).

Lets create some directories, for instance, a/b/c/d:

```
# mount disk.img disk/
# mkdir -p disk/a/b/c/d
# umount disk/
```

Using `fatcat`, we can dive into these directorie:

```
# fatcat disk.img -l /
d 25/10/2013 12:37:36  A/                             c=3
# fatcat disk.img -l /A
d 25/10/2013 12:37:36  B/                             c=4
# fatcat disk.img -l /A/B
d 25/10/2013 12:37:36  C/                             c=5
# fatcat disk.img -l /A/B/C
d 25/10/2013 12:37:36  D/                             c=6
```

As you can see, A begins in the cluster 3, B in the cluster 4, C in the
cluster 5 and D in the cluster 6.

Each directory reference its child with its cluster number. For instance,
in `/` you can see a reference to `A/` which is in the cluster 3.

Now, imagine you change the reference of `D` to 3, you would then go
back to A, right? Let's do it using `-e`:

```
# fatcat disk.img -e /A/B/C/D -c 3
Searching entry for /A/B/C/D
Entry address 00000000000c9640
Found entry, cluster=6, directory
Setting the cluster to 3
```

Now, have a look to the result:

```
# mount 
# ls disk/A/B/C/D/B/C/D/B/C/D/B/C/D/B/C/D/B/C/D/B/C/D/B/C/D/...
```

You made it! You created an infinite loop of directories. The directory structure
is no longer a tree, but a graph.

### The infinite file

Let's create a file which is longer than 1 cluster:

```
# mount disk.img disk/
# for i in `seq 0 100`; do echo "Hello world" >> disk/file.txt; done
# ls -lAh disk/file.txt
-rw-r--r-- 1 root root 1212 Oct 25 12:58 disk/file.txt
# umount disk.img
```

Now, have a look to it with `fatcat`:

```
# fatcat disk.img -e /file.txt
Searching entry for /file.txt
Entry address 00000000000c9020
Found entry, cluster=3, file with size=1212 (1.18359K)

# fatcat disk.img -@ 3
Cluster 3 address:
823808 (00000000000c9200)
Next cluster:
FAT1: 4 (00000004)
FAT2: 4 (00000004)
Chain size: 3
```

This means that our file is in cluster 3, its physical address in the file is
0xc9200 and its entry address in the file is 0xc9020. The file is 3 clusters
long, so there is 3 clusters "linked" to each others to represent the file.
The file size is 1212 bytes.

What if we link, for instance the second cluster to the first one and change
the file size?

To do this, we will need to write the FAT table, if we follow the chain to
the 4, we see:

```
# fatcat disk.img -@ 4
Cluster 4 address:
824320 (00000000000c9400)
Next cluster:
FAT1: 5 (00000005)
FAT2: 5 (00000005)
Chain size: 2
```

So, cluster 3 is linked to 4 that is linked to 5. Let's link cluster 4 to cluster 3:

```
# fatcat disk.img -w 4 -v 3
Writing next cluster of 4 from 5 to 3
Writing on FAT1
Writing on FAT2
```

Our file is now looping in the clusters. But the system will only read the size
available in its entry, we can now change it to whatever we want, if it's smaller
than 4GB since the size is coded in 32bits:

```
# fatcat disk.img -e /file.txt -s 4000000000
```

Let's try this out:

```
# mount disk.img disk/
# ls -lAh disk/file.txt 
-rwxr-xr-x 1 root root 3.8G Oct 25 13:00 disk/file.txt
# cat disk/file.txt |head
Hello world
Hello world
Hello world
Hello world
Hello world
Hello world
Hello world
...
```

You did it! You build a file that seems to be 3.8G but is in fact just occupying
some bytes of your disk and looping on itself.
