// InProgressFiles.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <string>

using namespace std;

int MAX_PARTICLES;
string name[];
float mass[];
float diameter[];
int brightness[];
int xcoord[];
int ycoord[];
int zcoord[];



//THINGS FROM SAMPLE
struct PARTICLE
{
	XMFLOAT4 pos; //position stored as a vector
	XMFLOAT4 velo; //to be calculated elsewhere?
};

PARTICLE* g_pParticleArray = NULL;


struct PARTICLE_VERTEX
{
	XMFLOAT4 Color;
};

//THINGS FROM SAMPLE

struct PARTICLE_DETAILS
{
	string name;
	float mass;
	float diameter;
	int brightness;
};

PARTICLE_DETAILS* g_pParticleArrayTWO = NULL;

//Loads all details about each object except positon and velocity
void fillParticleDetails(PARTICLE_DETAILS particles2[], int length, string nameIn[], float massIn[], float diameterIn[], int brightnessIn[]){
	for (int i = 0; i < length; i++){
		particles2[i].name = nameIn[i];
		particles2[i].mass = massIn[i];
		particles2[i].diameter = diameterIn[i];
		particles2[i].brightness = brightnessIn[i];
	}
}

void fillParticles(){
	//method that loads g_pParticleArray with position as a vector and velocity (that is being updated constantly?)
	//getPosition(origin x, origin y, origin z, particle x, particle y, particle z) and getVelocity methods 
}



/////////set of get methods for object properties/////////////

//gets the name of an object using its index number
string getName(int index){
	return g_pParticleArrayTWO[index].name;
}

//gets the mass of an object using its index number
float getMass(int index){
	return g_pParticleArrayTWO[index].mass;
}

//gets the diameter of an object using its index number
float getDiameter(int index){
	return g_pParticleArrayTWO[index].diameter;
}

//gets the mass of an object using its index number
int getBrightness(int index){
	return g_pParticleArrayTWO[index].brightness;
}

//gets position of object using its index number
positionType getPosition(int index){				//change positionType to what the actual type is
	return g_pParticleArray[index].pos;
}

//gets velocity of the object using its index number 
velocityType getVelocity(int index){				//change velocityType to what the actual type is
	return g_pParticleArray[index].velo;
}

/////////////////////////////////////////////////////

int _tmain(int argc, _TCHAR* argv[])
{
	g_pParticleArray = new PARTICLE[MAX_PARTICLES];
	g_pParticleArrayTWO = new PARTICLE_DETAILS[MAX_PARTICLES];

	fillParticleDetails(g_pParticleArrayTWO, MAX_PARTICLES, name, mass, diameter, brightness);
	return 0;
}