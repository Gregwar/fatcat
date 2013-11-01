<?php

/**
 * Unit testing fatcat tool
 */
class FatcatTests extends \PHPUnit_Framework_TestCase
{
    /**
     * Testing that the usage appears
     */
    public function testUsage()
    {
        $result = `fatcat`;

        $this->assertContains('fatcat', $result);
        $this->assertContains('Usage', $result);
        $this->assertContains('-i', $result);

        $this->assertEquals($result, `fatcat -h`);
    }

    /**
     * Testing informations about an image
     */
    public function testInfos()
    {
        $infos = `fatcat /tmp/empty.img -i`;

        $this->assertContains('FAT32', $infos);
        $this->assertContains('mkdosfs', $infos);
        $this->assertContains('Bytes per cluster: 512', $infos);
        $this->assertContains('Fat size: 403456', $infos);
        $this->assertContains('Data size: 51642368', $infos);
        $this->assertContains('Disk size: 52428800', $infos);
    }

    /**
     * Testing listing a directory
     */
    public function testListing()
    {
        $listing = `fatcat /tmp/hello-world.img -l /`;
        $this->assertContains('hello.txt', $listing);
        $this->assertContains('files/', $listing);
        
        $listing = `fatcat /tmp/hello-world.img -l /files/`;
        $this->assertContains('other_file.txt', $listing);

        $listing = `fatcat /tmp/hello-world.img -l /xyz 2>&1`;
        $this->assertContains('Error', $listing);
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
        $this->assertContains('FATs are exactly equals', $diff);

        $diff = `fatcat /tmp/repair.img -2`;
        $this->assertContains('FATs differs', $diff);
        $this->assertContains('It seems mergeable', $diff);
        
        $merge = `fatcat /tmp/repair.img -m`;
        $this->assertContains('Merging cluster 32', $merge);

        $diff = `fatcat /tmp/repair.img -2`;
        $this->assertContains('FATs are exactly equals', $diff);
    }
}
