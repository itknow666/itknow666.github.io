// #include：引入头文件的指令
// stdio.h 是标准输入输出库，里面声明了 printf、getchar 等函数
#include <stdio.h>

// main 是程序的主函数，程序运行时从这里开始执行
// int 表示这个函数会返回一个整数
int main(){
	// printf：输出函数，把双引号里的内容打印到屏幕上
	// \n 可以换成换行，这里不加的话输出后不会换行
	printf("helloworld");

	// getchar：等待用户按下一个键才继续往下执行
	// 作用是让程序暂停，防止控制台窗口一闪而过
	getchar();

	// return 0：程序正常结束，把 0 返回给操作系统
	// 约定 0 表示程序运行成功
	return 0;
}
