#include <stdio.h>
#include <stdlib.h>

int main()
{
    system("chcp 65001 >nul");   // 把控制台切换到 UTF-8 代码页，中文才能正常显示
    int num;

    printf("请输入一个十进制整数：");
    scanf("%d", &num);

    printf("十进制：%d\n", num);
    printf("八进制：%o\n", num);
    printf("十六进制：%x\n", num);

    return 0;
}
