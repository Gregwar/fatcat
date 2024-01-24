<?php

/**
 * Unit testing fatcat tool
 */
class FatcatTests extends \PHPUnit\Framework\TestCase
{
    /**
     * Testing that the usage appears
     */
    public function testUsage()
    {
        $result = `fatcat`;

        $this->assertStringContainsString('fatcat', $result);
        $this->assertStringContainsString('Usage', $result);
        $this->assertStringContainsString('-i', $result);

        $this->assertEquals($result, `fatcat -h`);
    }

    /**
     * Testing informations about an image
     */
    public function testInfos()
    {
        $infos = `fatcat /tmp/empty.img -i`;

        $this->assertStringContainsString('FAT32', $infos);
        $this->assertStringContainsString('mkdosfs', $infos);
        $this->assertStringContainsString('Bytes per cluster: 512', $infos);
        $this->assertStringContainsString('Fat size: 403456', $infos);
        $this->assertStringContainsString('Data size: 51642368', $infos);
        $this->assertStringContainsString('Disk size: 52428800', $infos);
    }

    /**
     * Testing listing a directory
     */
    public function testListing()
    {
        $listing = `fatcat /tmp/hello-world.img -l /`;
        $this->assertStringContainsString('hello.txt', $listing);
        $this->assertStringContainsString('files/', $listing);
        
        $listing = `fatcat /tmp/hello-world.img -l /files/`;
        $this->assertStringContainsString('other_file.txt', $listing);

        $listing = `fatcat /tmp/hello-world.img -l /xyz 2>&1`;
        $this->assertStringContainsString('Error', $listing);
    }

    /**
     * Testing reading a file
     */
    public function testReading()
    {
        $file = `fatcat /tmp/hello-world.img -r /hello.txt`;
        $this->assertEquals("Hello world!\n", $file);
        
        $file = `fatcat /tmp/hello-world.img -R 3 -s 13`;
        $this->assertEquals("Hello world!\n", $file);
        
        $file = `fatcat /tmp/hello-world.img -r /files/other_file.txt`;
        $this->assertEquals("Hello!\nThis is another file!\n", $file);

        $file = `fatcat /tmp/hello-world.img -R 5 -s 29`;
        $this->assertEquals("Hello!\nThis is another file!\n", $file);
    }

    /**
     * Testing the -2 and -m
     */
    public function testDiff()
    {
        $diff = `fatcat /tmp/hello-world.img -2`;
        $this->assertStringContainsString('FATs are exactly equals', $diff);

        $diff = `fatcat /tmp/repair.img -2`;
        $this->assertStringContainsString('FATs differs', $diff);
        $this->assertStringContainsString('It seems mergeable', $diff);
        
        $merge = `fatcat /tmp/repair.img -m`;
        $this->assertStringContainsString('Merging cluster 32', $merge);

        $diff = `fatcat /tmp/repair.img -2`;
        $this->assertStringContainsString('FATs are exactly equals', $diff);
    }

    /**
     * Testing reading deleted files & dir
     */
    public function testDeleted()
    {
        $listing = `fatcat /tmp/deleted.img -l /`;
        $this->assertStringNotContainsString('deleted', $listing);
        
        $listing = `fatcat /tmp/deleted.img -l / -d`;
        $this->assertStringContainsString('deleted', $listing);
        
        $listing = `fatcat /tmp/deleted.img -l /deleted -d`;
        $this->assertStringContainsString('file.txt', $listing);

        $file = `fatcat /tmp/deleted.img -r /deleted/file.txt 2>/dev/null`;
        $this->assertEquals("This file was deleted!\n", $file);
    }

    /**
     * Testing backuping & restoring FAT
     */
    public function testBackupFAT()
    {
        $sum = md5_file('/tmp/hello-world.img');

        `fatcat /tmp/hello-world.img -b /tmp/hello-world.fat`;

        $this->assertTrue(file_exists('/tmp/hello-world.fat'));

        $fat = md5_file('/tmp/hello-world.fat');
        $this->assertEquals('c1eefa66f9da00f542f224dcb68c90c4', $fat);

        `fatcat /tmp/hello-world.img -p /dev/zero`;

        $sum2 = md5_file('/tmp/hello-world.img');
        $this->assertNotEquals($sum, $sum2);
        
        `fatcat /tmp/hello-world.img -p /tmp/hello-world.fat`;
        
        $sum3 = md5_file('/tmp/hello-world.img');
        $this->assertEquals($sum, $sum3);
    }
}
