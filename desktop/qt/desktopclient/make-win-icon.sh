#!/bin/bash
if [ ! -f images/bitmaps/icon-logo.ico ]; then
    (magick convert images/sources/icon-logo.svg -compress zip images/bitmaps/icon-logo.ico)2>&1>&/dev/null
fi
echo images/bitmaps/icon-logo.ico
