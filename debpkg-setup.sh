#!/bin/bash

set -x -e -o pipefail

export DEBEMAIL='shane@alcock.co.nz'
export DEBFULLNAME='Shane Alcock'
export DEBIAN_FRONTEND=noninteractive

apt-get update
apt-get install -y equivs devscripts dpkg-dev quilt curl apt-transport-https \
    apt-utils ssl-cert ca-certificates gnupg lsb-release debhelper git \
    pkg-config sed wget sudo software-properties-common

DISTRO=$(lsb_release -sc)

curl -1sLf 'https://dl.cloudsmith.io/public/wand/libwandio/cfg/setup/bash.deb.sh' | bash

case ${DISTRO} in
        bullseye | bookworm )
                sed -i 's/ dh-systemd (>=1.5),//' debian/control
        ;;

        jammy )
                sed -i 's/ dh-systemd (>=1.5),//' debian/control
        ;;

esac


apt-get update
apt-get upgrade -y
