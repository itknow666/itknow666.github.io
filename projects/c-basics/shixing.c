#include <stdio.h>

int main()
{
    double a = 3.14;    // 普通写法
    double b = -0.001;  // 负数
    double c = 12.;     // 小数点写在末尾，合法，等价于 12.0
    double d = .5;      // 小数点写在开头，合法，等价于 0.5

    printf("3.14    = %f\n", a);
    printf("-0.001  = %f\n", b);
    printf("12.     = %f\n", c);
    printf(".5      = %f\n", d);

    return 0;
}
