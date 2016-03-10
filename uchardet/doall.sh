#!/bin/bash

./makeARMV5T.sh
./makeARMV7.sh
./makeI686.sh
./makeMIPSEL.sh
./makeSH4.sh

./makeARMV5T.sh static
./makeARMV7.sh  static
./makeI686.sh   static
./makeMIPSEL.sh static
./makeSH4.sh    static
