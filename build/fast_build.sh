#!/bin/bash
set -ex

# exports
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig

# variables
USER=fastocloud

# update system
apt-get update
apt-get install -y git python3-setuptools python3-pip mongodb --no-install-recommends

# sync modules
git submodule update --init --recursive

# install pyfastogt
git clone https://github.com/fastogt/pyfastogt
cd pyfastogt
python3 setup.py install
cd ../
rm -rf pyfastogt

# build env for service
./build_env.py

# build service
LICENSE_KEY=$(license_gen)
./build.py release $LICENSE_KEY

# add user
useradd -m -U -d /home/$USER $USER -s /bin/bash
