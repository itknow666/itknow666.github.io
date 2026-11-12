#include <stdio.h>
#include <stdlib.h>
int main()
{
    system("chcp 65001 >nul");
    char c;
    scanf("%c", &c);
    printf("输入的字符是：%d\n", c);
    return 0;    
}