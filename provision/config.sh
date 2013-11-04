# Home
export COREDX_HOME="/opt/coredx/coredx-3.5.16-Evaluation"
export JAVA_HOME="/opt/java"
export LIBRARY_PATH="$COREDX_HOME/target/Linux_2.6_x86_64_gcc43/lib:$REGISTRY_HOME/provision/lib64:$REGISTRY_HOME/Registry/lib"
export PATH="$JAVA_HOME/bin:$PATH"

# Play
export PLAY_OPTS="-Xms256m -Xmx256m -Djava.library.path=$LIBRARY_PATH"

# Core
if [[ -d "$REGISTRY_HOME/provision" ]]
then
    export LD_PRELOAD="$REGISTRY_HOME/provision/lib64/libmacspoof.so.1.0.1"
    export TLM_LICENSE="$REGISTRY_HOME/provision/coredx/coredx.lic"
    mac="$(grep LICENSE $TLM_LICENSE | egrep -o 'HOSTID=[[:alnum:]]* ' | cut -d = -f 2)"
    export MAC_ADDRESS="${mac:0:2}:${mac:2:2}:${mac:4:2}:${mac:6:2}:${mac:8:2}:${mac:10:2}"
fi
