#!/bin/bash

set -x -e -o pipefail

export DEBEMAIL='shane@alcock.co.nz'
export DEBFULLNAME='Shane Alcock'
export DEBIAN_FRONTEND=noninteractive

git submodule init && git submodule update

mk-build-deps -i -r -t 'apt-get -f -y --force-yes'
dpkg-buildpackage -b -us -uc -rfakeroot -j4
