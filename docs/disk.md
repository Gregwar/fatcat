# FAT32 disk

Back to [documentation index](index.md)

To create a disk, you can use your favourite partition tool.

With linux tools, a common way to create a FAT32 empty image is:

```
# Create a 50M file full of zero
$ dd if=/dev/zero of=/disk.img bs=1M count=50
# Format it to FAT32
$ mkfs.vfat -F 32 disk.img
```

And that's it!
