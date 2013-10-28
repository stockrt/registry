#!/bin/bash

if [[ "$USER" != "lac" ]]
then
    echo "Execute este script na VM do LAC com o usuário \"lac\""
    exit 1
fi

pushd /home/$USER
wget -c -N http://downloads.typesafe.com/play/2.2.0/play-2.2.0.zip
unzip play-2.2.0.zip
popd

cat bashrc > /home/$USER/.bashrc

echo
echo
echo "#####################################################################################"
echo "Por favor abra um novo shell para carregar os comandos, digite 'alias' para conferir."
echo "#####################################################################################"
