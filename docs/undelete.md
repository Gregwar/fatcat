# FAT Undelete tutorial

Back to [documentation index](index.md)

First, [create a valid disk](disk.md), let's call it `disk.img`.

Let's mount the disk, create a file and unmount it (you need to
bee root to do this):

```
# mkdir disk
# mount disk.img disk/
# echo "Hello world" > disk/hello.txt
# umount disk
```

Using `fatcat`, you can explore the disk and see that `hello.txt` is
present:

```
# fatcat disk.img -l /
Listing path /
Directory cluster: 2
f 25/10/2013 11:05:28  hello.txt                      c=3 s=12 (12B)
```

You can also display the file contents using `-r`:

```
# fatcat disk.img -r /hello.txt
Hello world
```

Now let's delete this file:

```
# mount disk.img disk/
# rm disk/hello.txt
# umount disk
```

The file won't appear in `fatcat`:

```
# fatcat disk.img -l /
Listing path /
Directory cluster: 2
```

This is because you need to add the `-d` flag, to enable the display of
deleted files:

```
# fatcat disk.img -l / -d
Listing path /
Directory cluster: 2
f 25/10/2013 11:05:28  hello.txt                      c=3 s=12 (12B) d
```

`hello.txt` now appears with the "`d`" letter at the end. As you can see, the
cluster number and the filesize are still there.

You can now read it with exactly the same command as above:

```
# fatcat disk.img -r /hello.txt
! Trying to read a deleted file, auto-enabling contiguous mode
Hello world
```

As you can see, there is a message telling you that you are trying to read a deleted
file, this is because your deleted file may not be completely recovered since it has
to be contiguous in the drive.

