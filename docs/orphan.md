# FAT32 Orphan tutorial: how to recover lost files and directories

Back to [documentation index](index.md)

First, [create a valid disk](disk.md), let's call it `disk.img`.

We'll here explain the problem of orphan files in the FAT32 filesystems.

Sometimes, your disk may crash. It means that some sectors can be corrupted,
and thus unusable. Some parts of data are then broken.

But, there is not only data on your disk, but also directories. When directories
structure is broken, the files that were contained in it maybe not reachable anymore
by simply "walking" through the filesystem, this is what we call *orphan* files,
because they are alone and have no parents. Directories can be orphaned exactly the
same way, when the parent directory entry is corrupted.

We can also call it "lost files" or "lost directories", because they are still there,
but not reachable.

However, in FAT32 filesystems, the allocation table is stored in another place and
could remain intact. This mean that orphan files are perfectly readable, even more
than [deleted ones](undelete.md), because the cluster chains is still there.

Let's do a proof of concept of this, create a directory, and then a file in it:

```
# mount disk.img disk/
# mkdir disk/somedir/
# echo "Hello world" > disk/somedir/orphan.txt
# umount disk
```

Now, with `fatcat`, you are being able to list the files and to access `orphan.txt`:

```
# fatcat disk.img -l /
Listing path /
Directory cluster: 2
d 25/10/2013 11:20:54  somedir/                       c=3

# fatcat disk.img -l /somedir/
Listing path /somedir/
Directory cluster: 3
d 25/10/2013 11:20:30  ./                             c=3
d 25/10/2013 11:20:30  ../                            c=0
f 25/10/2013 11:20:54  orphan.txt                     c=4 s=12 (12B)

# fatcat disk.img -r /somedir/orphan.txt
Hello world
```

Here, the `somedir` directory is stored in the cluster 3, while the `orphan.txt`
is in the cluster 4.

With the `-e` option, we can have information about the entry of `somedir` itself:

```
# fatcat disk.img -e /somedir/
Searching entry for /somedir/
Entry address 00000000000c9020
Found entry, cluster=3, directory
```

This means that the `somedir` entry, which is in the list of directories and files
present in `/`, is stored in offset 0xc9020 in the `disk.img` image.

Now, imagine this entry is corrupted, for instance, the cluster reference could become
bad. Let's simulate this using `-e` and `-c` to change the cluster reference to another
value:

```
# fatcat disk.img -e /somedir/ -c 1234
Searching entry for /somedir/
Entry address 00000000000c9020
Found entry, cluster=3, directory
Setting the cluster to 1234
```

Now, if you mount your disk and have a look into somedir, you won't be able to find the
`orphan.txt` file anymore:

```
# mount disk.img disk/
# ls -lAh disk/somedir/
I/O error
# umount disk.img
```

This could happen if your disk was hardwarely corrupted. The problem is that the `orphan.txt`
file is still here, somewhere, but it's all alone. Actually, there is also another orphan
element here, the `somedir` directory. Even if we broke its entry, its listing also still exists.

You can now use one of the most powerful tool of `fatcat`, the orphan research. This analyses 
the allocated clusters and compares it to reacheable documents. Then, it will guess what cluster
contains directories and try to group the orphan files into orphan directories to have the less
elements as possible to show you:

```
# fatcat disk.img -o
Computing FAT cache...
Building the chains...
Found 3 chains

Running the recursive differential analysis...
Exploring 2
Exploring 0

Having a look at the chains...
Exploring 3
There is 1 orphaned elements:
* Directory clusters 3 to 3: 2 elements, 12B

Estimation of orphan files total sizes: 12 (12B)

Listing of found elements with known entry:
In directory with cluster 3:
f 26/10/2013 11:20:54  orphan.txt                     c=4 s=12 (12B)
```

The interesting section is the orphans list. As you can see, our cluster 3
appears in this list. Moreover, `fatcat` says that there is 2 elements regrouped
in this row, because there is the directory and the orphaned file. Just below, you
can also see a part of the listing showing you the name of the elements found.

You can now try to list the directory using `-L` and 3, because it's the cluster
number of the lost directory:

```
# fatcat disk.img -L 3
Listing cluster 3
Directory cluster: 3
d 25/10/2013 11:20:30  ./                             c=3
d 25/10/2013 11:20:30  ../                            c=0
f 25/10/2013 11:20:54  orphan.txt                     c=4 s=12 (12B)
```

And you get back the orphan directory! You can even read the file, using `-R`:

```
# fatcat disk.img -R 4 -s 12
Hello world
```

Or extract the orphan directory, using `-x` and `-f`:

```
# fatcat disk.img -x . -f 3
Extracting ./orphan.txt
# cat orphan.txt 
Hello world
```

Another way to get back your access to this orphaned directory would be
creating one and change its cluster to 3 in order to be able entering it again:

```
# mount disk.img disk/
# mkdir disk/tunnel
# umount disk.img
# fatcat disk.img -e /tunnel -c 3
# mount disk.img disk/
# ls -lAh disk/tunnel/
total 2
drwxr-xr-x 2 root root 512 Oct 25 11:40 ./
drwxr-xr-x 4 root root 512 Jan  1  1970 ../
-rwxr-xr-x 1 root root  12 Oct 25 11:20 orphan.txt*
```

This way, your `tunnel` directory will allow you to access again your files normally.

If you have a damaged disk, do not hesitate to test `-o`, this could be a really
good way to get some data back to life.
