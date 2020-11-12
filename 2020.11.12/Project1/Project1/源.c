#define _CRT_SECURE_NO_WARNINGS 1
#include<windows.h>
#include<stdio.h>
void QuBit(int x)
{
	int count = 0;
	while (x)
	{
		count++;
		x = x&(x - 1);
	}
	printf("%d\n", count);
}
int main()
{
	int m, n;
	scanf("%d%d", &m, &n);
	QuBit(m);
	QuBit(n);
	system("pause");
	return 0;
}