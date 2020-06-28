#include <Windows.h>                              // Header File For Windows
#include <GL\GL.h>                                // Header File For The OpenGL32 Library
#include <GL\GLU.h>                               // Header File For The GLu32 Library
#include <GL\freeglut.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>
#include <iostream>
#include <list>

using namespace std;

// Danh sách menu
enum MENU_TYPE
{
	MENU_HELP,
	MENU_SOLVE,
	MENU_INCREASE_SPEED,
	MENU_DECREASE_SPEED,
	MENU_Exit,
};

class Point {
public:
	double x;
	double y;
	double z;
	Point()
	{
		x = 0.0;
		y = 0.0;
		z = 0.0;
	}
	Point(double Set_X, double Set_Y, double Set_Z)
	{
		x = Set_X;
		y = Set_Y;
		z = Set_Z;
	}
	Point& operator= (Point const& disk)
	{
		x = disk.x;
		y = disk.y;
		z = disk.z;
		return *this;
	}
	Point operator-(Point const& disk)
	{
		return Point(x - disk.x, y - disk.y, z - disk.z);
	}
};

class MyDisk
{
public:
	MyDisk()
	{
		normal = Point(0.0, 0.0, 1.0);
	}
	Point position; //Vị trí hiện tại
	Point normal;   //Hướng
};

struct Activedisk {    //Chuẩn bị chuyển đĩa
	int disk_index; //Số thứ tự dĩa
	Point start_pos, dest_pos; //Vị trí đầu/Vị trí đến
	double u;		    // u E [0, 1]
	double step_u;
	bool is_in_motion;
	int direction;     // +1 cho trái, -1 qua phải, 0 giữ nguyên
};

// Số đĩa và chiều cao trục
const int NUM_DISCS = 5;
const double AXIS_HEIGHT = 3.0;

struct Axis {
	Point positions[NUM_DISCS]; //Vị trí đĩa
	int occupancy_val[NUM_DISCS]; //Số đĩa phía dưới đĩa hiện tại
};

struct GameBoard {
	double x_min, y_min, x_max, y_max; //Hệ tọa độ xy
	double axis_base_rad;               //Tọa độ cầu
	Axis axis[3]; //3 trục
};

struct solution_pair {
	int f, t;         //f = from, t = to
};


//Cài đặt bảng game
MyDisk disks[NUM_DISCS];
GameBoard t_board;
Activedisk active_disk;
list<solution_pair> sol;
bool to_solve = false;
MENU_TYPE show = MENU_HELP;

//Màn hình và frame
int FPS = 60; // Frame/giây
int prev_time = 0;
int window_width = 600, window_height = 600;

void initialize();
void initialize_game();
void display_handler();
void reshape_handler(int w, int h);
void keyboard_handler(unsigned char key, int x, int y);
void anim_handler();
void move_disk(int from_axis, int to_axis);
Point get_inerpolated_coordinate(Point v1, Point v2, double u);
void solve(); // Giải thuật
void menu(int);

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(window_width, window_height);
	glutInitWindowPosition(500, 10);
	glutCreateWindow("Towers of Hanoi");
	initialize();
	cout << "Solving Towers of Hanoi" << endl;
	cout << endl << "Press H for Help" << endl;


	// Tạo menu
	glutCreateMenu(menu);

	// Các menu items
	glutAddMenuEntry("Help (H)", MENU_HELP);
	glutAddMenuEntry("Solve (S)", MENU_SOLVE);
	glutAddMenuEntry("Increase Speed (F)", MENU_INCREASE_SPEED);
	glutAddMenuEntry("Decrease Speed (L)", MENU_DECREASE_SPEED);
	glutAddMenuEntry("Exit (Q, Esc)", MENU_Exit);

	// Click chuột phải
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	//Callbacks
	glutDisplayFunc(display_handler);
	glutReshapeFunc(reshape_handler);
	glutKeyboardFunc(keyboard_handler);
	glutIdleFunc(anim_handler);

	glutMainLoop();
	return 0;
}

void initialize()
{
	glClearColor(0.1, 0.1, 0.1, 5.0); // Màu BG
	glShadeModel(GL_SMOOTH);		  // Độ SMOOTH
	glEnable(GL_DEPTH_TEST);		  // Chiều sâu

	//Cài đặt độ sáng
	GLfloat light0_pos[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);


	prev_time = glutGet(GLUT_ELAPSED_TIME);

	//Bắt đầu vẽ màn hình game
	initialize_game();
}

void initialize_game()
{

	// Vẽ bảng game
	t_board.axis_base_rad = 1.0;
	t_board.x_min = 0.0;
	t_board.x_max = 6.6 * t_board.axis_base_rad;
	t_board.y_min = 0.0;
	t_board.y_max = 2.2 * t_board.axis_base_rad;

	double x_center = (t_board.x_max - t_board.x_min) / 2.0;
	double y_center = (t_board.y_max - t_board.y_min) / 2.0;

	double dx = (t_board.x_max - t_board.x_min) / 3.0;
	double r = t_board.axis_base_rad;

	//Cài đặt Occupancy 		
	for (int i = 0; i < 3; i++)
	{
		for (int h = 0; h < NUM_DISCS; h++)
		{
			if (i == 0)
			{
				t_board.axis[i].occupancy_val[h] = NUM_DISCS - 1 - h;
			}
			else t_board.axis[i].occupancy_val[h] = -1;
		}
	}

	//Vị trí trục
	for (int i = 0; i < 3; i++)
	{
		for (int h = 0; h < NUM_DISCS; h++)
		{
			double x = x_center + ((int)i - 1) * dx;
			double y = y_center;
			double z = (h + 1) * 0.3;
			Point& pos_to_set = t_board.axis[i].positions[h];
			pos_to_set.x = x;
			pos_to_set.y = y;
			pos_to_set.z = z;
		}
	}

	// Cài đặt đĩa
	for (int i = 0; i < NUM_DISCS; i++)
	{
		disks[i].position = t_board.axis[0].positions[NUM_DISCS - i - 1];
	}

	// Cài đặt dĩa actived (chuẩn bị chuyển)
	active_disk.disk_index = -1;
	active_disk.is_in_motion = false;
	active_disk.step_u = 0.02;
	active_disk.u = 0.0;
	active_disk.direction = 0;
}

//Vẽ cylinder biết vị trí, radian và chiều cao
void DrawAxe(double x, double y, double r, double h)
{
	GLUquadric* q = gluNewQuadric();
	GLint slices = 50;
	GLint stacks = 10;
	glPushMatrix();
	glTranslatef(x, y, 0.0f);
	gluCylinder(q, r, r, h, slices, stacks);
	glTranslatef(0, 0, h);
	gluDisk(q, 0, r, slices, stacks);
	glPopMatrix();
	gluDeleteQuadric(q);
}

//Vẽ bảng game và trục
void DrawBoardAndAxis(GameBoard const& board)
{

	GLfloat mat_white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat mat_yellow[] = { 1.0f, 1.0f, 0.0f, 1.0f };
	glPushMatrix();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_white);
	glBegin(GL_QUADS);
	glNormal3f(0.0f, 0.0f, 1.0f);
	glVertex2f(board.x_min, board.y_min);
	glVertex2f(board.x_min, board.y_max);
	glVertex2f(board.x_max, board.y_max);
	glVertex2f(board.x_max, board.y_min);
	glEnd();

	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_yellow);

	double r = board.axis_base_rad;
	for (int i = 0; i < 3; i++)
	{
		Point const& p = board.axis[i].positions[0];
		DrawAxe(p.x, p.y, r * 0.1, AXIS_HEIGHT - 0.1);
		DrawAxe(p.x, p.y, r, 0.1);
	}

	glPopMatrix();
}

// Vẽ đĩa
void draw_disks()
{
	int slices = 100;
	int stacks = 10;
	double rad;
	GLfloat r, g, b;
	GLfloat emission[] = { 0.4f, 0.4f, 0.4f, 1.0f };
	GLfloat no_emission[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	GLfloat material[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	int color = 0;
	GLfloat factor = 1.6f;
	factor = 0.1;
	for (size_t i = 0; i < NUM_DISCS; i++)
	{
		if (color >= 6) color -= 6;
		switch (color)
		{
		case 0: r = 1.0f; g = 0.0f; b = 0.0f;
			break;
		case 1: r = 0.0f; g = 1.0f; b = 0.0f;
			break;
		case 2: r = 0.0f, g = 0.0f; b = 1.0f;
			break;
		case 3: r = 1.0f, g = 1.0f; b = 0.0f;
			break;
		case 4: r = 0.0f, g = 1.0f; b = 1.0f;
			break;
		case 5: r = 1.0f, g = 0.0f; b = 1.0f;
			break;
		default: r = g = b = 1.0f;
			break;
		};
		color++;
		material[0] = r;
		material[1] = g;
		material[2] = b;
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, material);

		GLfloat u = 0.0f;

		if (i == active_disk.disk_index)
		{
			//Chỉnh sáng
			glMaterialfv(GL_FRONT, GL_EMISSION, emission);
			u = active_disk.u;
		}
		rad = factor * t_board.axis_base_rad;
		int d = active_disk.direction;

		glPushMatrix();
		glTranslatef(disks[i].position.x, disks[i].position.y, disks[i].position.z);
		double theta = acos(disks[i].normal.z);
		theta *= 180.0f / M_PI;
		glRotatef(d * theta, 0.0f, 1.0f, 0.0f);
		glutSolidTorus(0.2 * t_board.axis_base_rad, rad, stacks, slices);
		glPopMatrix();

		glMaterialfv(GL_FRONT, GL_EMISSION, no_emission);
		factor += 0.2;
	}
}

void display_handler()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	double x_center = (t_board.x_max - t_board.x_min) / 2.0;
	double y_center = (t_board.y_max - t_board.y_min) / 2.0;
	double r = t_board.axis_base_rad;
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(x_center, y_center - 10.0, 3.0 * r, x_center, y_center, 3.0, 0.0, 0.0, 1.0);
	glPushMatrix();
	DrawBoardAndAxis(t_board);
	draw_disks();
	glPopMatrix();
	glFlush();
	glutSwapBuffers();
}

void reshape_handler(int w, int h)
{
	window_width = w;
	window_height = h;
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (GLfloat)w / (GLfloat)h, 0.1, 20.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

class Stack
{
	/*
		disk: số đĩa
		x: trục hiện tại
		y: trục trung gian
		z: trục đến
		k: điều kiện dừng
	*/
	int *st, *disk, *x, *y, *z, *k;
	int size;
	void Init(int sz);
	void CleanUp();
public:
	Stack(int sz = 20)
	{
		Init(sz);
	}
	~Stack()
	{
		CleanUp();
	}
	bool Full() const;
	bool Empty() const;
	bool Push(int a, int b, int c, int d, int e);
	bool Pop(int *a, int *b, int *c, int *d, int *e);
};

void Stack::Init(int sz)
{
	st = disk = new int[sz];
	x = new int[sz];
	y = new int[sz];
	z = new int[sz];
	k = new int[sz];
	size = sz;
}

void Stack::CleanUp()
{
	if (st)
		delete[]st;
	if (x)
		delete[]x;
	if (y)
		delete[]y;
	if (z)
		delete[]z;
	if (k)
		delete[]k;
}

bool Stack::Full() const
{
	return (disk - st >= size);
}

bool Stack::Empty() const
{
	return (disk <= st);
}



bool Stack::Push(int a, int b, int c, int d, int e)
{
	if (Full())
	{
		Stack Temp(size);
		while (!Empty())
		{
			*disk--;
			*x--;
			*y--;
			*z--;
			*k--;
			Temp.Push(*disk, *x, *y, *z, *k);
		}
		CleanUp();
		Init(2 * size);
		while (!Temp.Empty())
		{
			*Temp.disk--;
			*Temp.x--;
			*Temp.y--;
			*Temp.z--;
			*Temp.k--;
			Push(*Temp.disk, *Temp.x, *Temp.y, *Temp.z, *Temp.k);
		}
	}
	*disk++ = a;
	*x++ = b;
	*y++ = c;
	*z++ = d;
	*k++ = e;
	return true;
}

bool Stack::Pop(int *a, int *b, int *c, int *d, int*e)
{
	if (Empty())
		return false;
	*disk--;
	*x--;
	*y--;
	*z--;
	*k--;
	*a = *disk;
	*b = *x;
	*c = *y;
	*d = *z;
	*e = *k;
	return true;
}

void Move_Stack(Stack &S, int disk, int x, int y, int z)
{
	int k = 0;
	S.Push(disk, x, y, z, 1);
	do
	{
		while (disk > 0)
		{
			S.Push(disk, x, y, z, 2);
			disk--;
			int temp = y;
			y = z;
			z = temp;
		}
		int a, b, c, d, e;
		S.Pop(&a, &b, &c, &d, &e);
		disk = a;
		x = b;
		y = c;
		z = d;
		k = e;
		if (k == 2)
		{
			solution_pair s;
			s.f = x;
			s.t = z;
			sol.push_back(s);
			disk--;
			int temp = x;
			x = y;
			y = temp;
		}
	} while (k != 1);
	cout << "The number of move is  " << pow(2, disk) - 1 << endl;
}

void solve()
{
	Stack S;
	Move_Stack(S, NUM_DISCS, 0, 1, 2);
}

//Thao tác phím máy tính
void keyboard_handler(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27:
	case 'q':
	case 'Q':
		exit(0);
		glutAttachMenu(GLUT_RIGHT_BUTTON);
		break;
	case 'h':
	case 'H':
		cout << "Q: Quit" << endl;
		cout << "S: Solve" << endl;
		cout << "H: Help" << endl;
		cout << "F: increase Speed" << endl;
		cout << "L: decrease Speed" << endl;
		cout << "-----------------------------" << endl;
		glutAttachMenu(GLUT_RIGHT_BUTTON);
		break;
	case 's':
	case 'S':
		if (t_board.axis[0].occupancy_val[NUM_DISCS - 1] < 0)
			break;
		solve();
		to_solve = true;
		glutAttachMenu(GLUT_RIGHT_BUTTON);
		break;
	case 'F':
	case 'f':
		if (FPS > 490) FPS = 500;
		else FPS += 10;
		cout << "(+) Speed: " << FPS / 5.0 << "%" << endl;
		glutAttachMenu(GLUT_RIGHT_BUTTON);
		break;
	case 'L':
	case 'l':
		if (FPS <= 10)FPS = 10;
		else FPS -= 10;
		cout << "(-) Speed: " << FPS / 5.0 << "%" << endl;
		glutAttachMenu(GLUT_RIGHT_BUTTON);
		break;
	default:
		break;
	};
}

void move_disk(int from_axis, int to_axis)
{

	int d = to_axis - from_axis;
	if (d > 0) active_disk.direction = 1;
	else if (d < 0) active_disk.direction = -1;

	if ((from_axis == to_axis) || (from_axis < 0) || (to_axis < 0) || (from_axis > 2) || (to_axis > 2))
		return;

	size_t i;
	for (i = NUM_DISCS - 1; i >= 0 && t_board.axis[from_axis].occupancy_val[i] < 0; i--);
	if ((i < 0) || (i == 0 && t_board.axis[from_axis].occupancy_val[i] < 0))
		return; //index < 0 hoặc index = 0 và occupancy < 0 => trục rỗng

	active_disk.start_pos = t_board.axis[from_axis].positions[i];

	active_disk.disk_index = t_board.axis[from_axis].occupancy_val[i];
	active_disk.is_in_motion = true;
	active_disk.u = 0.0;


	size_t j;
	for (j = 0; j < NUM_DISCS - 1 && t_board.axis[to_axis].occupancy_val[j] >= 0; j++);
	active_disk.dest_pos = t_board.axis[to_axis].positions[j];

	t_board.axis[from_axis].occupancy_val[i] = -1;
	t_board.axis[to_axis].occupancy_val[j] = active_disk.disk_index;
}

Point get_inerpolated_coordinate(Point sp, Point tp, double u)
{

	Point p;
	double x_center = (t_board.x_max - t_board.x_min) / 2.0;
	double y_center = (t_board.y_max - t_board.y_min) / 2.0;

	double u3 = u * u * u;
	double u2 = u * u;

	Point cps[4]; //P1, P2, dP1, dP2


					//Hermite Interpolation [Check Reference for equation of spline]
	{
		//P1
		cps[0].x = sp.x;
		cps[0].y = y_center;
		cps[0].z = AXIS_HEIGHT + 0.2 * (t_board.axis_base_rad);

		//P2
		cps[1].x = tp.x;
		cps[1].y = y_center;
		cps[1].z = AXIS_HEIGHT + 0.2 * (t_board.axis_base_rad);

		//dP1
		cps[2].x = (sp.x + tp.x) / 2.0 - sp.x;
		cps[2].y = y_center;
		cps[2].z = 2 * cps[1].z; //change 2 * ..

								 //dP2
		cps[3].x = tp.x - (tp.x + sp.x) / 2.0;
		cps[3].y = y_center;
		cps[3].z = -cps[2].z; //- cps[2].z;


		double h0 = 2 * u3 - 3 * u2 + 1;
		double h1 = -2 * u3 + 3 * u2;
		double h2 = u3 - 2 * u2 + u;
		double h3 = u3 - u2;

		p.x = h0 * cps[0].x + h1 * cps[1].x + h2 * cps[2].x + h3 * cps[3].x;
		p.y = h0 * cps[0].y + h1 * cps[1].y + h2 * cps[2].y + h3 * cps[3].y;
		p.z = h0 * cps[0].z + h1 * cps[1].z + h2 * cps[2].z + h3 * cps[3].z;

	}

	return p;
}

void normalize(Point& v)
{
	double length = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	if (length == 0.0) return;
	v.x /= length;
	v.y /= length;
	v.z /= length;
}

void anim_handler()
{
	int curr_time = glutGet(GLUT_ELAPSED_TIME);
	int elapsed = curr_time - prev_time;
	if (elapsed < 1000 / FPS) return;

	prev_time = curr_time;

	if (to_solve && active_disk.is_in_motion == false) {
		solution_pair s = sol.front();
		sol.pop_front();
		int i;
		for (i = NUM_DISCS; i >= 0 && t_board.axis[s.f].occupancy_val[i] < 0; i--);
		int ind = t_board.axis[s.f].occupancy_val[i];

		if (ind >= 0)
			active_disk.disk_index = ind;

		move_disk(s.f, s.t);
		if (sol.size() == 0)
			to_solve = false;
	}

	if (active_disk.is_in_motion)
	{
		int ind = active_disk.disk_index;
		Activedisk& ad = active_disk;

		if (ad.u == 0.0 && (disks[ind].position.z < AXIS_HEIGHT + 0.2 * (t_board.axis_base_rad)))
		{
			disks[ind].position.z += 0.05;
			glutPostRedisplay();
			return;
		}

		static bool done = false;
		if (ad.u == 1.0 && disks[ind].position.z > ad.dest_pos.z)
		{
			done = true;
			disks[ind].normal = Point(0, 0, 1);
			disks[ind].position.z -= 0.05;
			glutPostRedisplay();
			return;
		}

		ad.u += ad.step_u;
		if (ad.u > 1.0) {
			ad.u = 1.0;
		}

		if (!done) {
			Point prev_p = disks[ind].position;
			Point p = get_inerpolated_coordinate(ad.start_pos, ad.dest_pos, ad.u);
			disks[ind].position = p;
			disks[ind].normal.x = (p - prev_p).x;
			disks[ind].normal.y = (p - prev_p).y;
			disks[ind].normal.z = (p - prev_p).z;
			normalize(disks[ind].normal);
		}

		if (ad.u >= 1.0 && disks[ind].position.z <= ad.dest_pos.z) {
			disks[ind].position.z = ad.dest_pos.z;
			ad.is_in_motion = false;
			done = false;
			ad.u = 0.0;
			disks[ad.disk_index].normal = Point(0, 0, 1);
			ad.disk_index = -1;
		}
		glutPostRedisplay();
	}
}

//Xuất các menu
void menu(int item)
{
	switch (item)
	{
	case MENU_HELP:
		cout << "Q: Quit" << endl;
		cout << "S: Solve" << endl;
		cout << "H: Help" << endl;
		cout << "F: increase Speed" << endl;
		cout << "L: decrease Speed" << endl;
		cout << "-----------------------------" << endl;
		break;
	case MENU_SOLVE:
		if (t_board.axis[0].occupancy_val[NUM_DISCS - 1] < 0)
			break;
		solve();
		to_solve = true;
		break;
	case MENU_INCREASE_SPEED:
		if (FPS > 490) FPS = 500;
		else FPS += 10;
		cout << "(+) Speed: " << FPS / 5.0 << "%" << endl;
		break;
	case MENU_DECREASE_SPEED:
		if (FPS <= 10)FPS = 10;
		else FPS -= 10;
		cout << "(-) Speed: " << FPS / 5.0 << "%" << endl;
		break;
	case MENU_Exit:
		exit(0);
		break;
	default:
		break;
	}
	glutPostRedisplay();
	return;
}