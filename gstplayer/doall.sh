#!/bin/bash
set -e

rm -rf tmp
mkdir tmp

(cd gst-0.10 ; ./makeSH4.sh)
(cd gst-0.10 ; ./makeMIPSEL.sh) 
(cd gst-0.10 ; ./makeI686.sh)

cp -Rf gst-0.10/sh4 tmp/
cp -Rf gst-0.10/mipsel tmp/
cp -Rf gst-0.10/i686 tmp/

(cd gst-1.0 ; ./makeSH4.sh)
(cd gst-1.0 ; ./makeMIPSEL.sh) 
(cd gst-1.0 ; ./makeI686.sh) 
(cd gst-1.0 ; ./makeARMV7.sh) 
(cd gst-1.0 ; ./makeARMV5T.sh) 

cp -Rf gst-1.0/sh4 tmp/
cp -Rf gst-1.0/mipsel tmp/
cp -Rf gst-1.0/i686 tmp/
cp -Rf gst-1.0/armv7 tmp/
cp -Rf gst-1.0/armv5t tmp/

chmod -R 777 tmp

echo ">>>>>>>>>>>>>>>>>>>>>>>> DONE <<<<<<<<<<<<<<<<<<<<<<<<"
exit 