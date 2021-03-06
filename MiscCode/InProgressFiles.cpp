// InProgressFiles.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <string>

using namespace std;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define GET_OBJECT_INFO 8



int NUM_PARTICLES;
string name[];
float mass[];
float diameter[];
float brightness[];
float xcoord[];
float ycoord[];
float zcoord[];



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
	float brightness;
};

PARTICLE_DETAILS* g_pParticleArrayTWO = NULL;

//Loads all details about each object except positon and velocity
void fillParticleDetails(PARTICLE_DETAILS particles2[], int num Particles, string nameIn[], float massIn[], float diameterIn[], float brightnessIn[]){
	for (int i = 0; i < maxParticles; i++){
		particles2[i].name = nameIn[i];
		particles2[i].mass = massIn[i];
		particles2[i].diameter = diameterIn[i];
		particles2[i].brightness = brightnessIn[i];
	}
}

void fillParticles(PARTICLE particles[], int maxParticles, float x[], float y[], float z[], XMFLOAT4 Velocity){
	//method that loads g_pParticleArray with position as a vector and velocity (that is being updated constantly?)
	//create and then call here a getVelocity methods 

	for (int i = 0; i < maxParticles; i++){
		particles[i].pos = createPositionFloat(x[i], y[i], z[i]);
		pParticles[i].velo = Velocity;
	}
}



fillParticles(g_pParticleArray, 5, xcoord, ycoord, zcoord, XMFLOAT4(0, 0, 0, 1));



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
float getBrightness(int index){
	return g_pParticleArrayTWO[index].brightness;
}

//gets position of object using its index number
XMFLOAT4 getPosition(int index){				//change positionType to what the actual type is
	return g_pParticleArray[index].pos;
}

//gets velocity of the object using its index number 
velocityType getVelocity(int index){				//change velocityType to what the actual type is
	return g_pParticleArray[index].velo;
}

/////////////////////////////////////////////////////

//method for establishing the position of a celestial body
//definition for XMFLOAT4 is in the sample, line 570
XMFLOAT4 createPositionFloat(float xParticle, float yParticle, float zParticle){
	return XMFLOAT4(xParticle, yParticle, zParticle, 10000.0f * 10000.0f);
}



/////////////////////////////////////////////////////

//BUTTONS

//method for creating buttons for each object
void createObjectButtons(int NUM_PARTICLES){
	for (int i = 0; i < NUM_PARTICLES; i++){
		LPCWSTR name = getName(i);
		g_HUD.AddButton(GET_OBJECT_INFO, Lname, getPosition(i).x, getPosition(i).y, 10, 10)
	}
} 

//action for CALLBACK when button is clicked
case GET_OBJECT_INFO:
	displayObjInfo(); break;

//method to display object info
	void displayObjInfo(int index){

	}

/////////////////////////////////////////////////////

int _tmain(int argc, _TCHAR* argv[])
{
	g_pParticleArray = new PARTICLE[MAX_PARTICLES];
	g_pParticleArrayTWO = new PARTICLE_DETAILS[MAX_PARTICLES];

	fillParticleDetails(g_pParticleArrayTWO, MAX_PARTICLES, name, mass, diameter, brightness);
	return 0;
}

