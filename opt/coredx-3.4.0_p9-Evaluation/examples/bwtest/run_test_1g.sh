#!/bin/sh
# ***************************************************************
#   
#   file:  run_test_1g.sh
#   desc:  Script to run the bw test (run_client.sh must be run
#          first).
#  
# ***************************************************************
#    
#    Coypright(C) 2009-2011 Twin Oaks Computing, Inc
#    All rights reserved.   Castle Rock, CO 80108
# 
# ****************************************************************
#   
#   This file is provided by Twin Oaks Computing, Inc 
#   as an example. It is provided in the hope that it will be
#   useful but WITHOUT ANY WARRANTY; without even the implied
#   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
#   PURPOSE. TOC Inc assumes no liability or responsibilty for
#   the use of this information for any purpose.
#   
# ***************************************************************/

./bwtest -p -l 10 -s 128   -w 100 -d  5  -b 1000  $@
./bwtest -p -l 10 -s 256   -w 100 -d  6  -b 500   $@
./bwtest -p -l 10 -s 512   -w 100 -d  6  -b 500   $@
./bwtest -p -l 10 -s 1024  -w 100 -d 10  -b 500   $@
./bwtest -p -l 10 -s 2048  -w  50 -d  5  -b 500   $@
./bwtest -p -l 10 -s 4096  -w 200 -d 10  -b 500   $@
./bwtest -p -l 10 -s 8192  -w 200 -d 10  -b 200   $@
./bwtest -p -l 10 -s 16384 -w   2 -d  6  -b 200   $@
./bwtest -p -l 10 -s 32768 -w   1 -d  1  -b 10    $@

