#!/bin/bash

set -e

# 如果没有build目录就创建一个
if [ ! -d "$(pwd)/build" ]; then
    mkdir "$(pwd)/build"
fi

# 清空build目录
rm -rf "$(pwd)/build/*"

# 进入build目录，运行cmake和make
cd "$(pwd)/build" && cmake .. && make

# 返回到项目根目录
cd ..

# 把头文件拷贝到 /usr/include/mymuduo  so文件拷贝到 /usr/lib
if [ ! -d /usr/include/myMuduo ]; then
    mkdir /usr/include/myMuduo
fi

for header in include/*.h
do
    cp "$header" /usr/include/myMuduo
done

cp "$(pwd)/lib/libmyMuduo.so" /usr/lib

# 更新共享库缓存
ldconfig