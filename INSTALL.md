# INSTALL
## Required
* Bison
## Install boost
1. `cd ./boost_1_85_0/`
2. `mkdir boost_build`
3. `./tools/build/b2 install --prefix=boost_build toolset=gcc`
4. `./tools/build/b2 --build_dir=build toolset=gcc --build-type=complete address-model=64 stage`

## reference
[ref 1](https://blog.csdn.net/zhizhengguan/article/details/96484543)