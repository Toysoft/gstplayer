#!/bin/bash

./makeARMV7.sh
./makeI686.sh
./makeMIPSEL.sh
./makeSH4.sh

./makeARMV7.sh  static
./makeI686.sh   static
./makeMIPSEL.sh static
./makeSH4.sh    static
