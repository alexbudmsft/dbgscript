#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct Foo
{
	int a, b, c;
	int arr[5];
	char name[100];
	wchar_t wide_name[100];
};

struct Bar
{
	Foo foo;
	float x, y;
};

struct Car
{
	Foo* f;
	Bar pie;
};

int main()
{
	Foo localFoo;
	Car car1, car2;
	int y = 6;
	
	car1.f = new Foo;
	memset(car1.f->arr, 'a', sizeof(car1.f->arr));
	car1.f->a = 6;
	car1.f->b = 12;
	car1.f->c = 245632;
	strcpy(car1.f->name, "AbcDef Hello");
	wcscpy(car1.f->wide_name, L"Wide String Test");
	car2 = car1;
	car2.pie.x = 5.3533;
	
	printf("hello, world! %d\n", y);
}