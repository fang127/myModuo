# myMuduo 网络库

## 项目简介
myMuduo 是一个基于 C++ 实现的高性能多线程网络库，参考 muduo 网络库设计理念，采用 Reactor 模式实现。该库旨在为 Linux 平台下的 TCP 网络应用开发提供高效、易用的异步事件驱动编程框架，帮助开发者快速构建高性能、可扩展的网络服务器程序。

## 功能特性
- ​Reactor 架构: 基于事件驱动的 Reactor 模式，支持高性能 I/O 多路复用

- 多线程支持: 内置线程池支持，可实现 one loop per thread 的高并发模型

- TCP 网络编程: 提供完整的 TCP 服务器/客户端编程接口

- 异步日志系统: 集成高性能异步日志模块，减少 I/O 操作对业务逻辑的影响

- 定时器功能: 支持高性能定时器管理，可用于超时控制和定时任务

- 缓冲区管理: 提供高效的自适应缓冲区设计，优化网络数据传输

## 系统环境
- 操作系统: Ubuntu 22.04 LTS
- 编译器: g++ 11.4 及以上版本
- C++ 标准: C++20 及以上
- CMake 版本: 3.10 及以上

## 编译安装
1. 准备依赖
	- g++ ≥ 11.4（支持 C++20）
	- CMake ≥ 3.10
	- `build-essential`、`cmake`、`libpthread` 等基础开发包（Ubuntu 可通过 `sudo apt install build-essential cmake` 获取）

2. 获取源码
	```bash
	git clone https://github.com/fang127/myMuduo.git
	cd myMuduo
	```

3. 一键构建与安装
	项目根目录提供了 `autobuild.sh` 脚本，会自动执行以下操作：
	- 生成并清理 `build/` 目录
	- 运行 `cmake` 与 `make` 构建共享库
	- 将头文件拷贝到 `/usr/include/myMuduo`
	- 将 `lib/libmyMuduo.so` 拷贝到 `/usr/lib` 并调用 `ldconfig`

	使用方式（需具备 `sudo` 权限以写入系统目录）：
	```bash
	chmod +x autobuild.sh
	sudo ./autobuild.sh
	```

4. 自定义/手动构建（可选）
	如果只想在本地目录下构建库，可手动执行：
	```bash
	mkdir -p build && cd build
	cmake ..
	make -j$(nproc)
	```
	构建完成后共享库位于 `lib/libmyMuduo.so`，若不希望安装到系统目录，可在运行依赖程序前临时设置：
	```bash
	export LD_LIBRARY_PATH=$(pwd)/../lib:$LD_LIBRARY_PATH
	```

5. 构建示例程序（可选）
	```bash
	cd ../example
	mkdir -p build && cd build
	cmake ..
	make -j$(nproc)
	export LD_LIBRARY_PATH=../../lib:$LD_LIBRARY_PATH
	./testserver
	```
	另开终端使用 `nc 127.0.0.1 8000` 即可进行回显测试。

## 贡献指南
欢迎提交Issue和Pull Request来帮助改进本项目。

**注意: 本项目为学习目的而开发，生产环境使用请进行充分测试**