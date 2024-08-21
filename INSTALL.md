# INSTALL
## Required
* Bison
* g++(std20, :warning:msvcrt REQUIRED)
## Install boost
1. `cd ./boost_1_85_0/`
<!-- 2. `mkdir boost_build` -->
3. `./tools/build/b2 --build_dir=build toolset=gcc variant=release,debug cxxflags=-std=gnu++20 cflags=-std=gnu20 link=static threading=multi runtime-link=static target-os=windows --build-type=complete address-model=64 stage`
## Install ncurses
1. go to [ncurses website](https://invisible-island.net/ncurses/#download_mingw) then download mingw port x64 pack.
2. unzip to porject root directory.
## Install cxxopts
1. go to [cxxpots github](https://github.com/jarro2783/cxxopts/releases) and download sourcecode
2. unzip to porject root directory.
## reference
[ref 1](https://blog.csdn.net/zhizhengguan/article/details/96484543)