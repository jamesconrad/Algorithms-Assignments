// Core Libraries
#include <iostream>
#include <string>
#include <math.h>

#include <vector>

// 3rd Party Libraries
#include <GLUT\glut.h>

// Defines and Core variables
#define FRAMES_PER_SECOND 60
const int FRAME_DELAY = 1000 / FRAMES_PER_SECOND; // Miliseconds per frame

class vec3f
{
public:
	float x, y, z;
	vec3f(float _x, float _y, float _z) { x = _x; y = _y; z = _z; }
	vec3f() { x = y = z = 0; }
	vec3f operator*(const vec3f& v) { return vec3f(x * v.x, y * v.y, z * v.z); }
	vec3f operator*(const float f) { return vec3f(x*f, y*f, z*f); }
	vec3f operator+(const vec3f& v) { return vec3f(x + v.x, y + v.y, z + v.z); }
};

int windowWidth = 800;
int windowHeight = 600;
int mousepositionX;
int mousepositionY;

// A few conversions to know
float degToRad = 3.14159f / 180.0f;
float radToDeg = 180.0f / 3.14159f;

struct ve
{
	vec3f v;
	bool e;
	ve(vec3f _v, bool _e) { v = _v; e = _e; }
};

struct ce
{
	vec3f v;
	float tval;
	float arcl;
	int seg;
};

//lerp vars
std::vector<vec3f> points;
std::vector<ve> ves;
vec3f obj(windowWidth / 2, windowHeight / 2 , 0);
vec3f objprev;
float time = 0;
int stage = 0;
bool paused = true;
bool loop = false;
int state = 1;
int cycles = 0;
std::vector<ce> curve;
bool curveComplete = true;
void interpolate(float time);
bool sc = true;

void dumpCurve()
{
	while (!curve.empty())
		curve.pop_back();
}

float arclength(int i)
{
	vec3f d = curve[i].v + (curve[i - 1].v * -1);
	float ad = sqrt(pow(d.x, 2) + pow(d.y, 2) + pow(d.z, 2));
	return ad + curve[i - 1].arcl;
}

void remakeCurve()
{
	dumpCurve();
	curveComplete = false;
	while (!curveComplete && points.size() > 1)
	{
		printf("Curve Loading: %i\n", curve.size());
		interpolate(FRAME_DELAY);
		ce c;
		c.v = obj;
		c.seg = stage;
		c.tval = time;
		c.arcl = 0;
		curve.push_back(c);
	}
	//distance function
	//mag(p2 - p1) + alp1
	for (int i = 1; i < curve.size(); i++)
	{
		curve[i].arcl = arclength(i);
	}

	for (int i = 0; i < curve.size(); i++)
	{
		curve[i].arcl /= curve.back().arcl;
	}
}

template <typename T>
T lerp(T d0, T d1, float t)
{
	return d0 * (1 - t) + d1 * t;
}

template <typename T>
float invlerp(T d0, T d1, T res)
{
	return (res + (d0*-1))/(d1 + (d0*-1));
}

vec3f infLerp(std::vector<vec3f>& vec, float time)
{
	std::vector<vec3f> v;
	if (vec.size() > 1)
	{
		for (int i = 0, s = vec.size() - 1; i < s; i++)
		{
			v.push_back(lerp(vec[i], vec[i + 1], time));
		}
		glBegin(GL_LINE_STRIP);
		glColor3f(0.5, 0.5, 0.5);
		for (int i = 0, s = v.size(); i < s; i++)
		{
			glVertex3f(v[i].x, v[i].y, 0);
		}
		glEnd();
		return infLerp(v, time);
	}
	else
		return vec[0];
}

void interpolate(float dTime)
{
	std::vector<vec3f> lerps;
	if (stage + 1 < points.size() && state == 1)
	{
		obj = lerp(points[stage], points[stage + 1], time);
	}

	if (stage + 2 < points.size() && state == 2)
	{
		//this is where it gets fun
		//step one lerp i to i+1, and i+1 to i+2
		lerps.push_back(lerp(points[stage], points[stage + 1], time));
		lerps.push_back(lerp(points[stage + 1], points[stage + 2], time));
		//and now we lerp a to b
		obj = lerp(lerps[0], lerps[1], time);
		//and finally we render our interval lerp lines here so we dont have to store the lerps any longer
		glBegin(GL_LINES);
		glColor3f(0.5, 0.5, 0.5);
		glVertex3f(lerps[0].x, lerps[0].y, lerps[0].z);
		glVertex3f(lerps[1].x, lerps[1].y, lerps[1].z);
		glEnd();

		while (!lerps.empty())
			lerps.pop_back();
	}

	if (stage + 1 < points.size() && state == 3)
	{
		//
		//float t, struct point p1, struct point p2, struct point p3, struct point p4
		float t = time;
		float t2 = time*time;
		float t3 = time*time*time;

		/* Catmull Rom spline Calculation */
		vec3f prev(0, 0, 0);
		vec3f last(0, 0, 0);
		if (stage == 0)
			prev = points[stage];
		else
			prev = points[stage - 1];

		if (stage + 2 == points.size())
			last = points[points.size() - 1];
		else
			last = points[stage + 2];
		obj.x = ((-t3 + 2 * t2 - t)*(prev.x) + (3 * t3 - 5 * t2 + 2)*(points[stage].x) + (-3 * t3 + 4 * t2 + t)* (points[stage + 1].x) + (t3 - t2)*(last.x)) / 2;
		obj.y = ((-t3 + 2 * t2 - t)*(prev.y) + (3 * t3 - 5 * t2 + 2)*(points[stage].y) + (-3 * t3 + 4 * t2 + t)* (points[stage + 1].y) + (t3 - t2)*(last.y)) / 2;
	}

	if (stage + 3 < points.size() && state == 4)
	{
		//lerp the first stage
		//i 0-1 == line 0
		lerps.push_back(lerp(points[stage], points[stage + 1], time));
		lerps.push_back(lerp(points[stage + 1], points[stage + 2], time));
		//i 1-2 == line 1
		lerps.push_back(lerp(points[stage + 2], points[stage + 3], time));

		//lerp the 2nd line
		//i 4 == final 0
		lerps.push_back(lerp(lerps[0], lerps[1], time));
		//i 5 == final 1
		lerps.push_back(lerp(lerps[1], lerps[2], time));

		//lerp the pos
		obj = lerp(lerps[3], lerps[4], time);

		//and finally we render our interval lerp lines here so we dont have to store the lerps any longer
		for (int i = 0; i < 4; i++)
		{
			if (i != 2)
			{
				glBegin(GL_LINES);
				glColor3f(0.5, 0.5, 0.5);
				glVertex3f(lerps[i].x, lerps[i].y, lerps[i].z);
				glVertex3f(lerps[i + 1].x, lerps[i + 1].y, lerps[i + 1].z);
				glEnd();
			}
		}

		while (!lerps.empty())
			lerps.pop_back();
	}


	//now for infinite bezier curves
	if (state == 5)
	{
		//step one get all prelim curves
		for (int i = 0, s = points.size() - 1; i < s; i++)
			lerps.push_back(lerp(points[i], points[i + 1], time));
		glBegin(GL_LINE_STRIP);
		glColor3f(0.5, 0.5, 0.5);
		for (int i = 0, s = lerps.size(); i < s; i++)
			glVertex3f(lerps[i].x, lerps[i].y, 0);
		glEnd();
		obj = infLerp(lerps, time);

		while (!lerps.empty())
			lerps.pop_back();
	}


	//Looping
	if (cycles >= points.size() - 1)
	{
		if (loop)
		{
			stage = 0;
			time = 0;
			cycles = 0;
		}
		curveComplete = true;
	}
	else if (state == 2)
	{
		if (points.size() % 2 == 0)
		{
			//last point is invalid
			if (cycles >= points.size() - 3)
			{
				if (loop)
				{
					stage = 0;
					time = 0;
					cycles = 0;
				}
				curveComplete = true;
			}
		}
		else if (cycles >= points.size() - 2)
		{
			if (loop)
			{
				stage = 0;
				time = 0;
				cycles = 0;
			}
			curveComplete = true;
		}
	}
	else if (state == 4)
	{
		if (cycles >= points.size() - 3)
		{
			if (loop)
			{
				stage = 0;
				time = 0;
				cycles = 0;
			}
			curveComplete = true;
		}
	}

	if (!paused || !curveComplete)
		time += (float)dTime / 1000;

	if (time >= 1.0f)
	{
		time = 0;
		if (state == 2)
		{
			stage += state;
			cycles += state;
		}
		else if (state == 4)
		{
			stage += 3;
			cycles += 3;
		}
		else
		{
			stage++;
			cycles++;
		}
	}
}

int objI = 0;
float vel = 0.05;
float d = 0;

void DisplayCallbackFunction(void)
{
	/* clear the screen */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity(); // clear our the transform matrix
	objprev = obj;

	//interpolate(FRAME_DELAY);

	//now for object movement

	if (sc && curve.size() > 1)
	{
		//stage 1 lerp for distance
		//d is incremented by vel and reset to 0
		float dst = lerp(0.0f, 1.0f, d);
		//stage 2 find closest points
		int p1 = 0, p2 = 0;
		for (int i = 1; i < curve.size(); i++)
		{
			if (curve[i].arcl > dst)
			{
				p2 = i;
				p1 = i - 1;
				break;
			}
		}

		float il = invlerp(curve[p1].arcl, curve[p2].arcl, dst);
		obj = lerp(curve[p1].v, curve[p2].v, il);

		d += vel;
		if (d > 1)
			d = 0;
	}
	else
	{
		interpolate(FRAME_DELAY);
	}


	glBegin(GL_LINE_STRIP);
	glColor3f(0,0,0);
	for (unsigned int i = 0, s = points.size() - 1; i < s; i++)
		glVertex3f(points[i].x, points[i].y,0);

	//this way if the lerp wont work with the last datapoint it will render in a faded colour
	if (state == 2 && points.size() % 2 == 0)
		glColor3f(0.3, 0.3, 0.3);
	if (state == 4 && (points.size() - 4) % 3 != 0)
		glColor3f(0.3, 0.3, 0.3);
	glVertex3f(points[points.size() - 1].x, points[points.size() - 1].y, 0);
	glEnd();

	glBegin(GL_POINTS);
	glColor3f(1, 1, 1);
	for (unsigned int i = 0, s = ves.size() - 1; i < s; i++)
		glVertex3f(ves[i].v.x, ves[i].v.y, 0);
	glEnd();

	glBegin(GL_QUADS);
	glColor3f(0.75f, 0.75f, 0.75f);
	glVertex3f(obj.x - 10, obj.y - 10, 0);
	glVertex3f(obj.x - 10, obj.y + 10, 0);
	glVertex3f(obj.x + 10, obj.y + 10, 0);
	glVertex3f(obj.x + 10, obj.y - 10, 0);
	glEnd();


	//curve.push_back(obj);

	glBegin(GL_LINE_STRIP);
	glColor3f(0, 0, 1);
	for (int i = 0; i < curve.size(); i++)
		glVertex3f(curve[i].v.x, curve[i].v.y, 0);
	glEnd();

	vec3f distance = obj + (objprev * -1);
	float delta = sqrt(pow(distance.x, 2) + pow(distance.y, 2) + pow(distance.z, 2));
	printf("Change in position this frame : %f\n", delta);

	glutSwapBuffers();
}

void reset()
{
	stage = 0;
	time = 0;
	obj = points[0];
	cycles = 0;
	remakeCurve();
}

void connect()
{
	while (!points.empty())
		points.pop_back();

	for (int i = 0, s = ves.size(); i < s; i++)
		if (ves[i].v.x > mousepositionX - 5 && ves[i].v.x < mousepositionX + 5)
			if (ves[i].v.y >windowHeight - mousepositionY - 5 && ves[i].v.y < windowHeight - mousepositionY + 5)
				ves[i].e = !ves[i].e;

	for (int i = 0, s = ves.size(); i < s; i++)
		if (ves[i].e == true)
			points.push_back(ves[i].v);

	if (points.size() == 0)
		points.push_back(obj);

	dumpCurve();
}

void KeyboardCallbackFunction(unsigned char key, int x, int y)
{
	switch (key)
	{
		case 32: // the space bar
			paused = !paused;
			break;
		case 'r':
			reset();
			break;
		case 'd':
			while (!points.empty())
				 points.pop_back();
			while (!ves.empty())
				ves.pop_back();
			points.push_back(obj);
			ves.push_back(ve(obj,true));
			stage = 0;
			time = 0;
			break;
		case 'o':
			loop = !loop;
			break;
		case '1':
			state = 1;
			reset();
			break;
		case 'f':
			connect();
			break;
		case '2':
			state = 2;
			reset();
			break;
		case '3':
			state = 3;
			reset();
			break;
		case '4':
			state = 4;
			reset();
			break;
		case '5':
			state = 5;
			reset();
			break;
		case '-':
			vel /= 2;
			break;
		case '=':
			vel *= 2;
			break;
		case 'c':
			sc = !sc;
			break;
		case 27: // the escape key
		case 'q': // the 'q' key
			exit(0);
			break;
	}
}

void TimerCallbackFunction(int value)
{
	glutPostRedisplay();
	glutTimerFunc(FRAME_DELAY, TimerCallbackFunction, 0);
}

void WindowReshapeCallbackFunction(int w, int h)
{
	float asp = (float)w / (float)h;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluOrtho2D(0, w, 0, h);

	windowWidth = w;
	windowHeight = h;
	glViewport(0, 0, windowWidth, windowHeight);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void MouseMoved(int x, int y)
{
	mousepositionX = x;
	mousepositionY = y;
}

void MouseClickCallbackFunction(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN)
	{
		points.push_back(vec3f(x, windowHeight - y, 0));
		ves.push_back(ve(vec3f(x, windowHeight - y, 0), true));
		remakeCurve();
	}
}

void init()
{
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_TEXTURE_2D);
}

int main(int argc, char **argv)
{
	/* initialize the window and OpenGL properly */
	glutInit(&argc, argv);
	glutInitWindowSize(windowWidth, windowHeight);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutCreateWindow("INFR1350U - Example");

	/* set up our function callbacks */
	glutDisplayFunc(DisplayCallbackFunction);
	glutKeyboardFunc(KeyboardCallbackFunction);
	glutReshapeFunc(WindowReshapeCallbackFunction);
	glutMouseFunc(MouseClickCallbackFunction);
	glutTimerFunc(1, TimerCallbackFunction, 0);
	glutPassiveMotionFunc(MouseMoved);

	init(); //Setup OpenGL States

	//fill vectors with obj to give first point
	points.push_back(obj);
	ves.push_back(ve(obj, true));

	/* start the event handler */
	glutMainLoop();
	return 0;
}