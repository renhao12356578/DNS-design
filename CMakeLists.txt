cmake_minimum_required(VERSION 3.10)
project(DNS_Relay_System C)

# Set C standard
set(CMAKE_C_STANDARD 99)


# 添加 -fcommon 标志来允许全局变量的多重定义
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-fcommon)
endif()

# 包含头文件 判断是否存在
include_directories(include)

# 源文件
set(SOURCES
    src/main.c
    src/dns_config.c
    src/dns_convert.c
    src/dns_server.c
    src/dns_mes_print.c
    src/dns_resetid.c
    src/dns_table.c
    src/dns_cache.c
    src/output_level.c
)

# 创建可执行文件
add_executable(dns_relay ${SOURCES})

# Link Windows Socket library
if(WIN32)
    target_link_libraries(dns_relay  ws2_32)
endif()

# 安装目标
install(TARGETS dns_relay DESTINATION bin)




