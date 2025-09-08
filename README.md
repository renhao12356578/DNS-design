# DNS中继系统 (DNS Relay System)

## 项目简介

这是一个基于C语言开发的DNS中继服务器，实现了DNS查询的本地缓存、hosts文件解析、以及与上游DNS服务器的通信功能。系统采用UDP协议进行网络通信，支持IPv4地址解析，具备完整的DNS报文解析和构造能力。

## 功能特性

### 核心功能
- 🌐 **DNS查询中继**：接收客户端DNS查询请求，转发至上游DNS服务器
- 📁 **本地hosts文件解析**：支持本地域名到IP地址的映射
- 💾 **智能缓存机制**：缓存DNS查询结果，提高响应速度和减少网络负载
- ⚡ **多种运行模式**：支持阻塞和非阻塞两种工作模式
- 📝 **详细日志记录**：提供多级别的日志输出和调试信息
- 🔄 **ID映射管理**：处理DNS查询ID的映射和转换，解决并发查询冲突

### 高级特性
- 🚫 **域名拦截**：支持通过0.0.0.0映射拦截特定域名
- 🔧 **灵活配置**：支持多种命令行参数配置
- 💨 **性能优化**：解决忙等待问题，优化CPU使用率
- 🌍 **多IP支持**：单个域名可映射和缓存多个IP地址
- ⏰ **TTL管理**：支持DNS记录的生存时间管理

## 系统要求

- **操作系统**：Windows (使用Winsock2 API)
- **编译器**：支持C99标准的编译器 (GCC, MSVC, Clang)
- **构建工具**：CMake 3.10+
- **依赖库**：ws2_32.lib (Windows Socket库)

## 编译安装

### 使用CMake编译

```bash
# 创建构建目录
mkdir build
cd build

# 配置项目
cmake ..

# 编译项目
cmake --build .

# 安装 (可选)
cmake --install .
```

### 手动编译

```bash
gcc -std=c99 -Iinclude src/*.c -lws2_32 -o dns_relay
```

## 使用方法

### 基本用法

```bash
# 启动DNS中继服务器 (默认配置)
./dns_relay

# 启用日志记录
./dns_relay -l

# 设置上游DNS服务器
./dns_relay -s 8.8.8.8

# 设置为阻塞模式
./dns_relay -m 1

# 指定hosts文件路径
./dns_relay -p /path/to/hosts.txt
```

### 命令行参数

| 参数 | 描述 | 示例 |
|------|------|------|
| `-l` | 启用日志记录 | `./dns_relay -l` |
| `-d` | 查看收发信息 | `./dns_relay -d` |
| `-dd` | 开启调试模式 | `./dns_relay -dd` |
| `-s [server]` | 设置远程DNS服务器地址 | `./dns_relay -s 8.8.8.8` |
| `-m [mode]` | 设置运行模式 (0=非阻塞, 1=阻塞) | `./dns_relay -m 1` |
| `-p [path]` | 设置hosts文件路径 | `./dns_relay -p ./my_hosts.txt` |

### 测试DNS服务器

启动服务器后，可以使用以下命令测试：

```bash
# 使用nslookup测试
nslookup www.example.com 127.0.0.1

# 使用dig测试 (Linux/Mac)
dig @127.0.0.1 www.example.com

# 使用ping测试
ping www.example.com
```

## 配置文件

### hosts文件格式

```
# 基本映射
192.168.1.100 myserver.local
127.0.0.1 localhost

# 多IP映射 (同一域名多行)
1.1.1.1 example.com
8.8.8.8 example.com

# 域名拦截 (映射到0.0.0.0)
0.0.0.0 ads.example.com
0.0.0.0 malware.site.com
```

### 默认配置

- **hosts文件路径**：`./hosts.txt`
- **日志文件路径**：`./log.txt`
- **默认DNS服务器**：`10.3.9.5`
- **默认运行模式**：非阻塞模式
- **监听端口**：53 (DNS标准端口)

## 项目结构

```
dns_relay_system/
├── CMakeLists.txt          # CMake构建配置
├── README.md               # 项目说明文档
├── design.md               # 详细设计报告
├── socket.md               # Socket编程说明
├── hosts.txt               # 默认hosts文件
├── include/                # 头文件目录
│   ├── dns_struct.h        # DNS数据结构定义
│   ├── dns_config.h        # 配置管理
│   ├── dns_server.h        # 服务器核心
│   ├── dns_convert.h       # 报文转换
│   ├── dns_cache.h         # 缓存管理
│   ├── dns_table.h         # hosts表管理
│   ├── dns_resetid.h       # ID映射管理
│   ├── dns_mes_print.h     # 调试输出
│   ├── output_level.h      # 日志级别
│   └── uthash.h            # 哈希表库
├── src/                    # 源文件目录
│   ├── main.c              # 程序入口
│   ├── dns_config.c        # 配置管理实现
│   ├── dns_server.c        # 服务器核心实现
│   ├── dns_convert.c       # DNS报文转换
│   ├── dns_cache.c         # 缓存管理实现
│   ├── dns_table.c         # hosts表实现
│   ├── dns_resetid.c       # ID映射实现
│   ├── dns_mes_print.c     # 调试输出实现
│   └── output_level.c      # 日志级别实现
├── test-server.c           # 测试服务器
├── test-client.c           # 测试客户端
└── build/                  # 构建输出目录
```

## 技术架构

### 系统架构图

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   DNS客户端     │    │   DNS中继服务器  │    │  上游DNS服务器   │
│  (nslookup等)   │    │  (本系统)       │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         │ 1. DNS查询请求         │                       │
         ├──────────────────────►│                       │
         │                       │ 2. 检查本地缓存/hosts  │
         │                       │                       │
         │                       │ 3. 转发查询(如需要)    │
         │                       ├──────────────────────►│
         │                       │                       │
         │                       │ 4. 接收响应           │
         │                       │◄──────────────────────┤
         │ 5. 返回DNS响应         │                       │
         │◄──────────────────────┤                       │
```

### 核心模块

1. **DNS协议处理模块** - 完整的DNS报文解析和构造
2. **网络通信模块** - UDP Socket管理和数据收发
3. **缓存管理模块** - 基于uthash的高效缓存系统
4. **Hosts文件管理** - 本地域名映射管理
5. **ID映射管理** - 解决并发查询ID冲突
6. **配置管理模块** - 命令行参数和系统配置

## 性能特性

### 忙等待优化

系统通过以下方式解决忙等待问题：

- **非阻塞模式**：添加`Sleep(1)`主动让出CPU，将CPU使用率从100%降至5-15%
- **阻塞模式**：使用`WSAPoll`事件驱动，CPU使用率降至1-5%
- **智能数据处理**：无数据时快速返回，避免无意义的处理

### 缓存性能

- 使用哈希表实现O(1)查找复杂度
- 支持TTL自动过期管理
- 定期清理过期条目释放内存
- 支持多IP地址缓存

## 支持的DNS记录类型

- **A记录** (Type=1) - IPv4地址映射
- **AAAA记录** (Type=28) - IPv6地址映射  
- **NS记录** (Type=2) - 域名服务器记录
- **CNAME记录** (Type=5) - 别名记录
- **SOA记录** (Type=6) - 权威记录起始
- **PTR记录** (Type=12) - 反向DNS查询
- **MX记录** (Type=15) - 邮件交换记录
- **TXT记录** (Type=16) - 文本记录

## 故障排除

### 常见问题

1. **端口占用错误**
   ```
   解决方案：确保端口53未被其他程序占用，或以管理员权限运行
   ```

2. **权限不足**
   ```
   解决方案：在Windows上以管理员权限运行程序
   ```

3. **hosts文件找不到**
   ```
   解决方案：检查hosts文件路径，或使用-p参数指定正确路径
   ```

4. **DNS查询超时**
   ```
   解决方案：检查上游DNS服务器地址是否正确，网络连接是否正常
   ```

### 调试模式

```bash
# 开启详细调试信息
./dns_relay -dd -l

# 查看日志文件
cat log.txt
```

## 开发说明

### 代码风格

- 遵循C99标准
- 使用4空格缩进
- 函数和变量使用驼峰命名
- 添加详细的注释说明

### 扩展开发

如需扩展功能，可以：

1. 添加新的DNS记录类型支持
2. 实现IPv6完整支持
3. 添加DNS over HTTPS (DoH) 支持
4. 实现负载均衡功能
5. 添加Web管理界面

## 许可证

本项目采用MIT许可证，详见LICENSE文件。

## 贡献

欢迎提交Issue和Pull Request！

### 贡献指南

1. Fork本项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 提交Pull Request

## 作者

- **RenHao** - 核心开发者
- **PengQuanYu** - 核心开发者

## 致谢

- 感谢uthash库提供的高效哈希表实现
- 感谢所有测试和反馈的用户

## 更新日志

### v1.0.0 (当前版本)
- ✅ 完整的DNS中继功能
- ✅ 智能缓存机制
- ✅ hosts文件支持
- ✅ 多运行模式
- ✅ 忙等待优化
- ✅ 详细日志记录

---

**如有问题或建议，请提交Issue或联系开发团队。**
