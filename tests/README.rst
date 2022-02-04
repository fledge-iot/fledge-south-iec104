*****************************************************
Unit Test for IEC 104 south plugin
*****************************************************

Require Google Unit Test framework

Install with:
::
    sudo apt-get install libgtest-dev
    cd /usr/src/gtest
    cmake CMakeLists.txt
    sudo make
    sudo make install

To build the unit tests:
::
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make
    ./RunTests