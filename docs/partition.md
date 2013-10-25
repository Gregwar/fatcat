# Use fatcat with a partitionned disk

Back to [documentation index](index.md)

Maybe you'll want to use fatcat with a partitionned drive. This is not
a problem. You just have to find out the right offset, and pass it to `-O`.

For instance, you can use fdisk to list the partitions:

```
# fdisk disk.img 

Command (m for help): p

Disk disk.img: 105 MB, 105906176 bytes
224 heads, 19 sectors/track, 48 cylinders, total 206848 sectors
Units = sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disk identifier: 0x00000000

   Device Boot      Start         End      Blocks   Id  System
   disk.img1            2048      206847      102400   83  Linux

Command (m for help): q
̀``

This output means that the first partition starts in the sector 2048. Each sector is 512 byte.
To use it with fatcat, you'll just have to pass -O to 2048*512=1048576 bytes:

```
fatcat disk.img -O 1048576 [options]
```

That's all!
