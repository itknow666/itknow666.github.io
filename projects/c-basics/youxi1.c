#include <stdio.h>
#include <stdlib.h>

int main()
{
    system("chcp 65001 >nul");

    char name[] = "Hero";    // 字符数组存角色名字
    int level = 1;           // 等级
    int hp = 100;            // 生命值
    int attack = 25;         // 攻击力

    printf("===== 角色创建成功 =====\n");
    printf("名字: %s\n", name);
    printf("等级: %d\n", level);
    printf("生命值: %d\n", hp);
    printf("攻击力: %d\n", attack);

    return 0;
}
