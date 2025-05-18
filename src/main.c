#include "../include/demo.h"
#include "../src/demo.c"
int main()
{
    int a,b;
    printf("请输入两个数：");
    scanf("%d %d",&a,&b);
    printf("%d\n",add(a,b));
    printf("%d\n",sub(a,b));
    printf("%d\n",mul(a,b));
    return 0;
}
