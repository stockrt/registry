#!/bin/bash

if [[ "$USER" != "lac" ]]
then
    echo "Execute este script na VM do LAC com o usuário \"lac\""
    exit 1
fi

sudo yum clean all
sudo yum -y install mysql-server
sudo /etc/init.d/mysqld restart
sudo chkconfig mysqld on
sudo mysql -p < ../sql/registry.sql
