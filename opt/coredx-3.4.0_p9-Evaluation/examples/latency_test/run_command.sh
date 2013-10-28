#!/bin/sh
# ***************************************************************
# 
#   file:  run_command.sh
#   desc:  Script to run the latency test
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

./latency_test_c -w 1 -s .01 -n 4 $@
sleep 1
./latency_test_c -w 1 -s .01 -n 8 $@
sleep 1
./latency_test_c -w 1 -s .01 -n 16 $@
sleep 1
./latency_test_c -w 1 -s .01 -n 32 $@
sleep 1
./latency_test_c -w 1 -s .01 -n 64 $@
sleep 1
./latency_test_c -w 1 -s .01 -n 128 $@
sleep 1
./latency_test_c -w 1 -s .01 -n 256 $@
sleep 1
./latency_test_c -w 1 -s .01 -n 512 $@
sleep 1
./latency_test_c -w 1 -s .01 -n 1024 $@
sleep 1
./latency_test_c -w 1 -s .01 -n 2048 $@
sleep 1
./latency_test_c -w 1 -s .01 -n 4096 $@
sleep 1
./latency_test_c -w 1 -s .01 -n 8192 $@
sleep 1
./latency_test_c -w 1 -s .01 -n 16384 $@

