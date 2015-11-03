#include <windows.h>
#include <stdio.h>
#include <strsafe.h>

#define STRING_AND_CCH(x) x, _countof(x)

struct Wheel
{
	float diameter;
};

struct Car
{
	int x, y;
	char name[100];
	WCHAR wide_name[100];
	Wheel wheels[4];
};

void beforeReturn()
{
	// Dummy function to break on.
	//
}

int main()
{
	Car car;
	
	car.x = 6;
	car.y = 10;
	
	StringCchCopyA(STRING_AND_CCH(car.name), "FooCar");
	StringCchCopyW(STRING_AND_CCH(car.wide_name), L"Wide FooCar");
	
	for (int i = 0; i < _countof(car.wheels); ++i)
	{
		car.wheels[i].diameter = 6.4643f;
	}
	
	beforeReturn();
	
	return 0;
}