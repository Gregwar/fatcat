# FAT32 repair guide

Back to [documentation index](index.md)

This guide try to describe the steps you can follow to try to fix a FAT32
filesystem that is damaged.

## Step 1: Backup everything!

First, you should backup your disk using `ddrescue` or another thing. You
won't be able to repair it it some sectors are physically damaged. You can either:

* Backup it to another disk
* Backup it to a file on your host machine (be careful, you'll need as much free
  space that the total size of your saved disk)

Then, you should backup your FAT tables to another file before trying to fix it
using fatcat, this can be done with `-b`:

```
fatcat disk.img -b backup.fat
```

This will backup your FAT tables in `backup.fat`. You can at any moment restore it with `-p`:

```
fatcat disk.img -p backup.fat
```

Note that all operations below will only change your FAT, not your data.

## Step 2: Try to merge your FAT tables

FAT tables in a broken disk could differ. You can compare them using `-2`:

```
fatcat disk.img -2
```

This will outputs the differences. You can then try to merge the tables using `-m`:

```
fatcat disk.img -m
```

This is not supposed to be very risked, and you could repair files using this.

## Step 3: Try to auto-fix the FAT

Fatcat can autofix some parts of your FAT tables, to this, you can try `-f`:

```
fatcat disk.img -f
```

This will walk your directories and search for entries (directories or files)
which are unallocated in the FAT. Then, it will try to fix the FAT to re-allocate
these entries, assuming its contiguous in the disk.

## Step 4: Find orphan files/directories

This last step will explore allocated files that are not reachable from your
directories.

You can use `-o` for this:

```
fatcat disk.img -o
```

For more information, have a look at the [orphaned files tutorial](orphan.md)

This step is the longest since you'll have to review manually its output and
try to read directories and files, hoping to get back important things.

Good luck!
