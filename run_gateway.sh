#!/bin/bash

# alias gateway='cd /home/lac/workspace/lib && java -Djava.library.path=.:/opt/coredx-3.4.0_p9-Evaluation/target/Linux_2.6_x86_gcc43/lib:.:/opt/coredx-3.4.0_p9-Evaluation/target/Linux_2.6_x86_gcc43/lib:/home/lac/workspace/lib -jar /home/lac/workspace/lib/contextnet.jar'

# libsigar-x86-linux.so
# https://support.hyperic.com/display/SIGAR/Home

export LD_PRELOAD="/opt/macspoof/libmacspoof.so.1.0.1"
export MAC_ADDRESS="00:1c:42:6b:f4:45"
export TLM_LICENSE="/opt/coredx-3.4.0_p9-Evaluation/CoreDXuniv-2012-08-01.lic"
cd /home/stockrt/Dropbox/stockrt/Roger/mestrado/cmu/sddl/cadastro/RegistryWeb/lib
java -Djava.library.path=".:/opt/coredx-3.4.0_p9-Evaluation/target/Linux_2.6_x86_gcc43/lib" -jar contextnet.jar 0.0.0.0 5500
