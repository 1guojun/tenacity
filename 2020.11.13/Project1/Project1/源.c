#define _CRT_SECURE_NO_WARNINGS 1
#include<windows.h>
#include<stdio.h>
int main()
{
	int a, b,c;
	scanf("%d%d", &a, &b);
	while (c = a%b)//��C=0ʱ�����������
	{
		a = b;
		b = c;
	}
	printf("%d\n", b);
	system("pause");
	return 0;
}