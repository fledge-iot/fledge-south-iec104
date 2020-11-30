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

git clone https://github.com/mz-automation/lib60870.git
cd lib60870/lib60870-C
cd dependencies
wget https://tls.mbed.org/download/mbedtls-2.6.0-apache.tgz
tar xf mbedtls-2.6.0-apache.tgz
cd ..
mkdir build
cd build
cmake ..
make
mv src/liblib60870.a lib60870.a
