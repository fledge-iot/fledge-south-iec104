#!/usr/bin/env bash

##--------------------------------------------------------------------
## Copyright (c) 2020 Dianomic Systems
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
##     http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.
##--------------------------------------------------------------------

##
## Author: Mark Riddoch
##

directory=$1
if [ ! -d $directory ]; then
  mkdir -p $directory
else
  directory=~
fi

if [ ! -d $directory/lib60870 ]; then
  cd $directory
  echo Fetching MZA lib60870 library
  git clone https://github.com/mz-automation/lib60870.git
  cd lib60870/lib60870-C
  cd dependencies
  wget https://tls.mbed.org/download/mbedtls-2.6.0-apache.tgz
  tar xf mbedtls-2.6.0-apache.tgz
  cd ..
  mkdir build
  cd build
  cmake -DBUILD_TESTS=NO -DBUILD_EXAMPLES=NO ..
  make
  sudo make install
  cd ..
  echo Set the environment variable LIB_104 to `pwd`
  echo export LIB_104=`pwd`
fi
