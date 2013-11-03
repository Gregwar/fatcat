<?php
/**
 * Copying the testing images to /tmp
 */

$images = array(
    'deleted', 'empty', 'hello-world', 'repair'
);
$directory = __DIR__ . '/../docs/images';

foreach ($images as $image) {
    $file = $image . '.img.gz';
    echo "Extracting $file...\n";
    `cp $directory/$file /tmp`;
    `gunzip -f /tmp/$file`;
}
