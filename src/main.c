#include"dns_struct.h"
#include"dns_config.h"

int main(int argc, char* argv[]) {
    /* 初始化系统 */
    // 将控制台的输出代码页设置为 UTF-8

    SetConsoleOutputCP(65001);
    
    configInit(argc, argv);

    /* 关闭连接 */
    closeSocketServer();

    return 0;
}