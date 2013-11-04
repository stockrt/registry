#!/bin/bash

if [[ "$HOSTNAME" != "registry" ]]
then
    echo "You must run this script on the VM, not in your host."
    exit
fi

if [[ "$USER" != "root" ]]
then
    echo "You must run this script as root (I need to install some packages)."
    exit
fi

# Config
echo
echo "- Configurando sistema..."
PLAY_VERSION="2.2.0"
REGISTRY_HOME=${REGISTRY_HOME:-"/vagrant"}
USER_APP=${USER_APP:-"vagrant"}
USER_HOME=${USER_HOME:-"/home/$USER_APP"}

if [[ ! -d "$REGISTRY_HOME" ]]
then
    echo "Diretório home do projeto não foi encontrado: $REGISTRY_HOME" 1>&2
    exit
fi
if [[ ! -d "$USER_HOME" ]]
then
    echo "Diretório home do usuário do projeto ($USER_APP) não foi encontrado: $USER_HOME" 1>&2
    exit
fi

echo "# Config" > /config.sh
echo "export PLAY_VERSION=\"$PLAY_VERSION\"" >> /config.sh
echo "export REGISTRY_HOME=\"$REGISTRY_HOME\"" >> /config.sh
echo "export USER_APP=\"$USER_APP\"" >> /config.sh
echo "export USER_HOME=\"$USER_HOME\"" >> /config.sh
echo >> /config.sh
cat $REGISTRY_HOME/provision/config.sh >> /config.sh

# Pacotes
echo
echo "- Instalando pacotes..."
DEBIAN_FRONTEND="noninteractive"
apt-get -f install
dpkg --configure -a
apt-get -y update
apt-get -y install build-essential linux-headers-$(uname -r)
apt-get -y install curl zip unzip

# MySQL
echo
echo "- Instalando MySQL..."
sudo debconf-set-selections <<< "mysql-server-<version> mysql-server/root_password password registry"
sudo debconf-set-selections <<< "mysql-server-<version> mysql-server/root_password_again password registry"
apt-get -y install mysql-server-5.5
service mysql restart
mysql --password=registry < $REGISTRY_HOME/provision/sql/registry_grant.sql

# CoreDX
echo
echo "- Instalando CoreDX..."
mkdir -p /opt/coredx
tar xzf $REGISTRY_HOME/provision/coredx/coredx-3.5.16-Linux_2.6_x86_64_gcc43-Evaluation.tgz -C /opt/coredx/

# Java
echo
echo "- Instalando Java..."
$REGISTRY_HOME/script/java-dl.sh >/dev/null 2>&1
chown -R ${USER_APP}: /opt/jdk*

# Play
echo
echo "- Instalando Play..."
cd $USER_HOME
wget -q -c http://downloads.typesafe.com/play/$PLAY_VERSION/play-${PLAY_VERSION}.zip
echo A | unzip -q play-${PLAY_VERSION}.zip >/dev/null 2>&1
chown -R ${USER_APP}: play*

# Aliases
echo
echo "- Instalando Aliases..."
cat $REGISTRY_HOME/provision/bashrc > $USER_HOME/.bashrc

# rc.local set
echo
echo "- Instalando rc.local..."
cat $REGISTRY_HOME/provision/rc.local > /etc/rc.local
chmod 755 /etc/rc.local

# rc.local run
echo
echo "- Executando rc.local..."
/etc/rc.local

# Message
echo
echo "- Aguarde enquanto são executados:
    - Gateway
    - Registry (Web Server e Core Server)
    - Migração do banco de dados
E então tente acessar: http://registry.vm"

# Wait
timeout=120
wtime=0
echo
echo "- Aguardando start dos processos por até $timeout segundos..."
while [[ $wtime -lt $timeout ]]
do
    flag_ok=1
    netstat -anl | grep -q :5500 || flag_ok=0
    netstat -anl | grep -q :9000 || flag_ok=0

    if [[ $flag_ok -eq 1 ]]; then break; fi

    sleep 1
    wtime=$((wtime+1))
done

# Done
sleep 5 # Aguardar alguns segundos devido ao bind
echo
if [[ $flag_ok -eq 1 ]]
then
    echo "- Pronto!
Já pode acessar: http://registry.vm"
else
    echo "- Problema no start dos processos. Por favor tente subir manualmente.
Utilize \"vagrant ssh\" e as aliases configurados no ambiente." 1>&2
fi
