#include <windows.h>
#include <stdio.h>

struct Wheel
{
	float diameter;
};

struct Car
{
	int x, y;
	
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
	
	for (int i = 0; i < _countof(car.wheels); ++i)
	{
		car.wheels[i].diameter = 6.4643f;
	}
	
	beforeReturn();
	
	return 0;
}