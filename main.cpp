#include <iostream>
#include <sstream>
#include <cstdlib>
#include <Windows.h>
#include <thread>
#include <cmath>
#include <random>

using namespace std;
static bool running = true;

typedef char s8;
typedef unsigned char u8;
typedef short s16;
typedef unsigned short u16;
typedef int s32;
typedef unsigned int u32;
typedef long s64;
typedef unsigned long u64;

inline int
clamp(int min, int val, int max) {
	if (val < min) return min;
	if (val > max) return max;
	return val;
}

struct Render_State {
	int height, width;
	void* memory;
	
	BITMAPINFO bitmap_info;
};

static Render_State render_state;

struct Button_State {
	bool is_down;
	bool changed;
};

enum {
	BUTTON_UP,
	BUTTON_DOWN,
	BUTTON_LEFT,
	BUTTON_RIGHT,
	BUTTON_ESC,
	BUTTON_F1,
	BUTTON_F2,
	BUTTON_COUNT, // Should be the last item
};

struct Input {
	Button_State buttons[BUTTON_COUNT];
};


// Render functions
static void
clear_screen(u32 color) {
	u32* pixel = (u32*)render_state.memory;
	for (int y = 0; y < render_state.height; y++) {

		for (int x = 0; x < render_state.width; x++)
		{
			*pixel++ = color; // using color			
		}

	}
}


static void
draw_rect_in_pixels(int x0, int y0, int x1, int y1, u32 color) {

	x0 = clamp(0, x0, render_state.width);
	x1 = clamp(0, x1, render_state.width);
	y0 = clamp(0, y0, render_state.height);
	y1 = clamp(0, y1, render_state.height);

	for (int y = y0; y < y1; y++) {
		u32* pixel = (u32*)render_state.memory + x0 + y * render_state.width;
		for (int x = x0; x < x1; x++)
		{
			*pixel++ = color;
		}

	}
}

static float render_scale = 0.01f;

static void
draw_rect(float x, float y, float half_size_x, float half_size_y, u32 color) {

	x *= render_state.height * render_scale;
	y *= render_state.height * render_scale;
	half_size_x *= render_state.height * render_scale;
	half_size_y *= render_state.height * render_scale;

	x += render_state.width / 2.f;
	y += render_state.height / 2.f;

	// Change to pixels
	int x0 = x - half_size_x;
	int x1 = x + half_size_x;
	int y0 = y - half_size_y;
	int y1 = y + half_size_y;

	draw_rect_in_pixels(x0, y0, x1, y1, color);
}

// Game logic
#define is_down(b) input->buttons[b].is_down
#define pressed(b) (input->buttons[b].is_down && input->buttons[b].changed)
#define released(b) (!input->buttons[b].is_down && input->buttons[b].changed)


float player_pos_x;
float player_pos_y;
float speed = 5.f; // units per second
const int xBlocksCount = 37; // 181
const int yBlocksCount = 19; // 91
int Blocks[xBlocksCount][yBlocksCount];

int block_pos_x;
int block_pos_y;

int prevBlock_pos_x;
int prevBlock_pos_y;
int startBlock_pos_x;
int startBlock_pos_y;
int endBlock_pos_x;
int endBlock_pos_y;

const int BallCount = 99;
int BallCounter = 1;
float ballSpeedMultiplicator = 300;
int ball_block_pos_x;
int ball_block_pos_y;

bool topDirectionMove;
bool bottomDirectionMove;
bool leftDirectionMove;
bool rightDirectionMove;
bool directionMove;

struct BallCoords {
	float ball_pos_x;
	float ball_pos_y;
	int xdirection;
	int ydirection;
	int ball_block_pos_x;
	int ball_block_pos_y;
};

BallCoords Balls[BallCount] = {};


static void
fillBorders()
{
	for (int i = 0; i < xBlocksCount; i++) {
		Blocks[i][0] = 3; // "3" = static block 
		Blocks[i][yBlocksCount - 1] = 3; // "3" = static block 
	}

	for (int j = 0; j < yBlocksCount; j++) {
		Blocks[0][j] = 3; // "3" = static block 
		Blocks[xBlocksCount - 1][j] = 3; // "3" = static block
	}

	for (int i = 1; i < xBlocksCount - 1; i++) {
		for (int j = 1; j < yBlocksCount - 1; j++) {
			Blocks[i][j] = 0;
		}
	}

}

static void
SetBallsCount(int ballcouter) {
	if (ballcouter == 0) {
		BallCounter++;
		ballSpeedMultiplicator = ballSpeedMultiplicator / 1.15;
	}
	else {
		BallCounter = ballcouter;
	}

}

static void
SetBallsStartPos() {

	for (int i = 0; i < BallCounter; i++) {

		int k, o, m = 1, n = 1;

		if (i % 2 == 0 || i == 0) {
			m = 1; n = 1;
			if (i % 2 == 0 && i != 0) { n = -n; }
		}
		else {
			m = -1; n = -1;
			if (i % 3 == 0) { n = -n; }
		}

		k = clamp(-80, 5 * i * m, 80);
		o = clamp(-40, 5 * i * n, 40);

		Balls[i].ball_pos_x = k;
		Balls[i].ball_pos_y = o;
		Balls[i].xdirection = m;
		Balls[i].ydirection = n;
	}
}

static void  // OBSOLETE
findPath(int MatchingBlockStatus, int newPathBlockStatus, int startPosX, int startPosY, int endPosX, int endPosY, bool setStartPosStatus, int clockwise, int ignoreBlockStatus) {
	// clockwise = 0, counterclockwise = 1, both = 2

	int i1 = startPosX;
	int j1 = startPosY;
	int a1 = 0;
	int b1 = 0;
	int k = 0;
	int m = 0;
	bool loop = true;
	bool IsPathFinded;



	if (setStartPosStatus && Blocks[a1][b1] == MatchingBlockStatus) {
		Blocks[i1][j1] = newPathBlockStatus;
	}



	for (int i = 0; i < 2; i++)
	{
		int localclockwise;
		if (clockwise == 0) { i = 2; localclockwise = 1; }
		if (clockwise == 1) { i = 2; localclockwise = -1; }
		if (clockwise == 2 && i == 0) { localclockwise = 1; }
		if (clockwise == 2 && i == 1) { localclockwise = -1; }

		int x1direction;
		int y1direction;

		while (loop) {
			IsPathFinded = false;
			i1 = clamp(0, i1, xBlocksCount - 1);
			j1 = clamp(0, j1, yBlocksCount - 1);

			bool directionselected = false;

			if (i1 <= endPosX && j1 <= endPosY && !directionselected) { x1direction = 1; y1direction = 1; directionselected = true; }
			if (i1 >= endPosX && j1 <= endPosY && !directionselected) { x1direction = -1; y1direction = 1; directionselected = true; }
			if (i1 >= endPosX && j1 >= endPosY && !directionselected) { x1direction = -1; y1direction = -1; directionselected = true; }
			if (i1 <= endPosX && j1 >= endPosY && !directionselected) { x1direction = 1; y1direction = -1; directionselected = true; }


			x1direction *= localclockwise;
			y1direction *= localclockwise;

			k = 0 * x1direction; m = 1 * y1direction;
			if ((j1 + m) < yBlocksCount) {
				a1 = clamp(0, i1 + k, xBlocksCount - 1);
				b1 = clamp(0, j1 + m, yBlocksCount - 1);
				if (Blocks[a1][b1] == (MatchingBlockStatus) && IsPathFinded == false) { IsPathFinded = true; Blocks[a1][b1] = newPathBlockStatus; i1 = a1; j1 = b1; }
			}

			k = 1 * x1direction; m = 0 * y1direction;
			if ((i1 + k) < xBlocksCount) {
				a1 = clamp(0, i1 + k, xBlocksCount - 1);
				b1 = clamp(0, j1 + m, yBlocksCount - 1);
				if (Blocks[a1][b1] == (MatchingBlockStatus) && IsPathFinded == false) { IsPathFinded = true; Blocks[a1][b1] = newPathBlockStatus; i1 = a1; j1 = b1; }
			}

			k = 0 * x1direction; m = -1 * y1direction;
			if ((j1 + m) >= 0) {
				a1 = clamp(0, i1 + k, xBlocksCount - 1);
				b1 = clamp(0, j1 + m, yBlocksCount - 1);
				if (Blocks[a1][b1] == (MatchingBlockStatus) && IsPathFinded == false) { IsPathFinded = true; Blocks[a1][b1] = newPathBlockStatus; i1 = a1; j1 = b1; }
			}

			k = -1 * x1direction; m = 0 * y1direction;
			if ((i1 + k) >= 0) {
				a1 = clamp(0, i1 + k, xBlocksCount - 1);
				b1 = clamp(0, j1 + m, yBlocksCount - 1);
				if (Blocks[a1][b1] == (MatchingBlockStatus) && IsPathFinded == false) { IsPathFinded = true; Blocks[a1][b1] = newPathBlockStatus; i1 = a1; j1 = b1; }
			}


			if ((i1 == endPosX) && (j1 == endPosY)) { loop = false; }

			if (IsPathFinded == true) {
				IsPathFinded = false;
			}
			else {
				if (Blocks[a1][b1] == ignoreBlockStatus) {
					i1 = a1; j1 = b1;
				}
				else {
					loop = false; // there are no path :(
				}

			}
		}
	}
}

static void // Recursive flood fill algorithm in four directions (https://en.wikipedia.org/wiki/Flood_fill)
fillAreaFromPos(int status, int newstatus, int posX, int posY) {

	if (Blocks[posX][posY] != status) {
		return;
	}
	else {
		Blocks[posX][posY] = newstatus;
		fillAreaFromPos(0, 4, posX - 1, posY);
		fillAreaFromPos(0, 4, posX + 1, posY);
		fillAreaFromPos(0, 4, posX, posY + 1);
		fillAreaFromPos(0, 4, posX, posY - 1);
	}

}


static void
restart(int ballcouter) {
	fillBorders();
	SetBallsCount(ballcouter);
	SetBallsStartPos();
	player_pos_x = -90.f;
	player_pos_y = -45.f;

	topDirectionMove = false;
	bottomDirectionMove = false;
	leftDirectionMove = false;
	rightDirectionMove = false;
	directionMove = false;
}

int difficulty = 0;


static void
simulate_game(Input* input, float dt) {

	if (pressed(BUTTON_ESC)) { running = false; }
	if (pressed(BUTTON_F1)) {
		difficulty = 0;
		restart(BallCounter);
	}

	if (pressed(BUTTON_F2)) {
		difficulty = 1;
		restart(BallCounter);
	}

	clear_screen(0x333333); // clear screen
	draw_rect(0, 0, 100, 100, 0x999999); // background
	draw_rect(0, 0, 87.5, 42.5, 0x666666); // background

	float isAttchedBlock = false;



	// Key input proccessing

	bool keyPressed = false;


	prevBlock_pos_x = clamp(0, (int)((player_pos_x)+90) / speed, xBlocksCount - 1);
	prevBlock_pos_y = clamp(0, (int)((player_pos_y)+45) / speed, yBlocksCount - 1);

	if (Blocks[prevBlock_pos_x][prevBlock_pos_y] == 3) {
		topDirectionMove = false;
		bottomDirectionMove = false;
		leftDirectionMove = false;
		rightDirectionMove = false;
		directionMove = false;
	}

	if (!directionMove) {
		if (pressed(BUTTON_UP) && player_pos_y < 45) { player_pos_y += speed; keyPressed = true; topDirectionMove = true; }
		if (pressed(BUTTON_DOWN) && player_pos_y > -45) { player_pos_y -= speed; keyPressed = true; bottomDirectionMove = true; }
		if (pressed(BUTTON_RIGHT) && player_pos_x < 90) { player_pos_x += speed; keyPressed = true; rightDirectionMove = true; }
		if (pressed(BUTTON_LEFT) && player_pos_x > -90) { player_pos_x -= speed; keyPressed = true; leftDirectionMove = true; }
		if (difficulty == 1) {
			directionMove = true;
		}
	}
	else {
		keyPressed = true;
		if (topDirectionMove) { player_pos_y += speed / 30; }
		if (bottomDirectionMove) { player_pos_y -= speed / 30; }
		if (rightDirectionMove) { player_pos_x += speed / 30; }
		if (leftDirectionMove) { player_pos_x -= speed / 30; }
	}





	// player postion in array coords
	block_pos_x = clamp(0, (int)((player_pos_x)+90) / speed, xBlocksCount - 1);
	block_pos_y = clamp(0, (int)((player_pos_y)+45) / speed, yBlocksCount - 1);

	// calc ball pos in block coords
	for (int i = 0; i < BallCounter; i++)
	{
		// ball position in pixels
		Balls[i].ball_pos_x += speed / ballSpeedMultiplicator * Balls[i].xdirection;
		Balls[i].ball_pos_y += speed / ballSpeedMultiplicator * Balls[i].ydirection;

		// ball position in array coords
		Balls[i].ball_block_pos_x = (int)(round(((Balls[i].ball_pos_x) + 90) / speed));
		Balls[i].ball_block_pos_y = (int)(round(((Balls[i].ball_pos_y) + 45) / speed));

	}


	if (keyPressed == false) {

		if (Blocks[block_pos_x][block_pos_y] == 3 && (
			(Blocks[block_pos_x - 1][block_pos_y] != 2 &&
				Blocks[block_pos_x + 1][block_pos_y] != 2 &&
				Blocks[block_pos_x][block_pos_y - 1] != 2 &&
				Blocks[block_pos_x][block_pos_y + 1] != 2)
			)) { // set block as start position for new line of blocks. "3" = static block 
			startBlock_pos_x = block_pos_x;
			startBlock_pos_y = block_pos_y;
		}

	}


	if (keyPressed = true) {

		endBlock_pos_x = block_pos_x;
		endBlock_pos_y = block_pos_y;

		// check "next" block for status
		if (Blocks[block_pos_x][block_pos_y] == 0) { // "0" = empty block

			// set new status for block
			if (Blocks[block_pos_x - 1][block_pos_y] >= 2 || // check left block status
				Blocks[block_pos_x + 1][block_pos_y] >= 2 || // check right block status
				Blocks[block_pos_x][block_pos_y - 1] >= 2 || // check bottom block status
				Blocks[block_pos_x][block_pos_y + 1] >= 2) { // check top block status

				Blocks[block_pos_x][block_pos_y] = 2; // "2" = set block as new attached				
			}
			else {
				Blocks[block_pos_x][block_pos_y] = 1; // "1" = set block as new detached block, if there are no connection to static or new attached block (connection lost)
			}



		}



		if (Blocks[block_pos_x][block_pos_y] == 3 &&
			(Blocks[block_pos_x - 1][block_pos_y] == 2 || Blocks[block_pos_x + 1][block_pos_y] == 2 ||
				Blocks[block_pos_x][block_pos_y - 1] == 2 || Blocks[block_pos_x][block_pos_y + 1] == 2)) {

			topDirectionMove = false;
			bottomDirectionMove = false;
			leftDirectionMove = false;
			rightDirectionMove = false;
			directionMove = false;


			// check line consistency
			int detachedBlocksCounter = 0;
			for (int i = 0; i < xBlocksCount - 1; i++) {
				for (int j = 0; j < xBlocksCount - 1; j++) {
					if (Blocks[i][j] == 1) {
						detachedBlocksCounter++;
						i = xBlocksCount;
						j = yBlocksCount;
					}
				}
			}

			// delete all new blocks if there are min 1 detached block
			if (detachedBlocksCounter > 0) {
				for (int i = 0; i < xBlocksCount; i++) {
					for (int j = 0; j < xBlocksCount; j++) {
						if (Blocks[i][j] <= 2) {
							Blocks[i][j] = 0; // debug = 1, release = 0
						}
					}
				}
			}

			// fill area if there are line closed
			if (detachedBlocksCounter == 0) {

				int detachedBallsCounter = 0;

				for (int b = 0; b < BallCounter; b++) {

					if (Blocks[Balls[b].ball_block_pos_x][Balls[b].ball_block_pos_y] == 4) {
						detachedBallsCounter++;
					}

					fillAreaFromPos(0, 4, Balls[b].ball_block_pos_x, Balls[b].ball_block_pos_y);

				}

				int filledCounter = 0;
				for (int i = 0; i < xBlocksCount; i++) {
					for (int j = 0; j < yBlocksCount; j++) {
						if (Blocks[i][j] == 0) {
							Blocks[i][j] = 3;
						}
						if (Blocks[i][j] >= 4) {
							Blocks[i][j] = 0;
						}
						if (Blocks[i][j] == 2) { // debug == 2, release >=2
							Blocks[i][j] = 3; // debug = 2, release = 3;
						}
						if (Blocks[i][j] == 3) {
							filledCounter++;
						}
					}
				}


				if (filledCounter >= 600 || detachedBallsCounter == 0) {
					restart(0);
				}


			}

		}
	}



	// Ball movement

	for (int i = 0; i < BallCounter; i++) {

		if (Blocks[Balls[i].ball_block_pos_x][Balls[i].ball_block_pos_y] >= 1) { // shoul be == 3. 1 and 2 for DEBUG
			if (Balls[i].ydirection == Balls[i].xdirection) { Balls[i].ydirection = (-1) * Balls[i].ydirection; }
			else { Balls[i].xdirection = (-1) * Balls[i].xdirection; }
		}

	}

	// Ball intersect new blocks

	for (int i = 0; i < BallCounter; i++) {
		if (Blocks[Balls[i].ball_block_pos_x][Balls[i].ball_block_pos_y] == 2 || Blocks[Balls[i].ball_block_pos_x][Balls[i].ball_block_pos_y] == 1) {
			restart(BallCounter);
		}
	}




	// Draw blocks
	for (int i = 0; i < xBlocksCount; i++) {
		for (int j = 0; j < yBlocksCount; j++) {
			int BlockStatusValue = Blocks[i][j];
			if (BlockStatusValue == 3) { draw_rect((i * speed - 90), (j * speed - 45), 2.1, 2.1, 0x222255); } // "3" = static block
			if (BlockStatusValue == 1) { draw_rect((i * speed - 90), (j * speed - 45), 2.3, 2.3, 0x992222); } // "1" = detached block
			if (BlockStatusValue == 2) { draw_rect((i * speed - 90), (j * speed - 45), 2.5, 2.5, 0x222288); } // "2" = new attached block 
			if (BlockStatusValue == 4) { draw_rect((i * speed - 90), (j * speed - 45), 1.5, 1.5, 0xffff00); } // "4" = filled block
			if (BlockStatusValue == 5) { draw_rect((i * speed - 90), (j * speed - 45), 1.5, 1.5, 0x00ff00); } // "5" = WIN
			if (BlockStatusValue == 6) { draw_rect((i * speed - 90), (j * speed - 45), 1.5, 1.5, 0xff0000); } // "6" = LOOSE


		}
	}

	// draw_rect(player_pos_x, player_pos_y, 1.9, 1.9, 0x000000); // draw player - BLACK

	for (int i = 0; i < BallCounter; i++) {
		draw_rect(Balls[i].ball_pos_x - 2.5, Balls[i].ball_pos_y - 2.5, 2.0, 2.0, 0x223322);	 // draw ball
	}

	// draw_rect((startBlock_pos_x * speed - 90) , (startBlock_pos_y * speed - 45) , 1.25, 1.25, 0xff0000); // draw start block (DEBUG) - RED
	draw_rect(endBlock_pos_x * speed - 90, endBlock_pos_y * speed - 45, 1.0, 1.0, 0x00ff00); // draw end block (DEBUG) - GREEN

	for (int i = 0; i < xBlocksCount; i++) {
		for (int j = 0; j < yBlocksCount; j++) {
			draw_rect((i * speed - 90), (j * speed - 45), 0.1, 0.1, 0xffffff);

		}
	}

}


LRESULT CALLBACK window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	LRESULT result = 0;
	switch (uMsg) {
		case WM_CLOSE:
		case WM_DESTROY: 
			{
				running = false;
			} break;
		
		case WM_SIZE: {
			RECT rect;
			GetClientRect(hwnd, &rect);
			render_state.width = rect.right - rect.left;
			render_state.height = rect.bottom - rect.top;
			
			int size = render_state.width*render_state.height*sizeof(unsigned int );
			
			if (render_state.memory) VirtualFree(render_state.memory, 0, MEM_RELEASE);
			render_state.memory = VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			
			render_state.bitmap_info.bmiHeader.biSize = sizeof(render_state.bitmap_info.bmiHeader);
			render_state.bitmap_info.bmiHeader.biWidth = render_state.width;
			render_state.bitmap_info.bmiHeader.biHeight = render_state.height;
			render_state.bitmap_info.bmiHeader.biPlanes = 1;
			render_state.bitmap_info.bmiHeader.biBitCount = 32;
			render_state.bitmap_info.bmiHeader.biCompression = BI_RGB;
			
			
		} break;
			
		default: 
			{
				result = DefWindowProc(hwnd, uMsg, wParam, lParam);
			}	
	}
	return result;
}


int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,LPSTR lpCmdLine, int nShowCmd) {
// Create Window Class
WNDCLASS window_class = {};
window_class.style = CS_HREDRAW | CS_VREDRAW;
window_class.lpszClassName = "Game Window Class";
window_class.lpfnWndProc = window_callback;
// Register Class
RegisterClass(&window_class);
// Create Window
HWND window = CreateWindowA(window_class.lpszClassName, "Game", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, 0, 0, hInstance, 0);
HDC hdc = GetDC(window);

Input input = {};

float delta_time = 0.016666f;
LARGE_INTEGER frame_begin_time;
QueryPerformanceCounter(&frame_begin_time);

float performance_frequency;
{
	LARGE_INTEGER perf;
	QueryPerformanceFrequency(&perf);
	performance_frequency = (float)perf.QuadPart;
}

restart(0);

while (running) {
		// Input
		MSG message;

		for (int i = 0; i < BUTTON_COUNT; i++) {
			input.buttons[i].changed = false;
		}



		while (PeekMessage(&message, window, 0, 0, PM_REMOVE)){

			switch (message.message)
			{
				case WM_KEYUP:
				case WM_KEYDOWN: {
						u32 vk_code = (u32)message.wParam;
						bool is_down = ((message.lParam & (1 << 31)) == 0);

						switch (vk_code) {
							case VK_UP: {
								input.buttons[BUTTON_UP].is_down = is_down;
								input.buttons[BUTTON_UP].changed = true;
							} break;
						}
						
						switch (vk_code) {
							case VK_DOWN: {
								input.buttons[BUTTON_DOWN].is_down = is_down;
								input.buttons[BUTTON_DOWN].changed = true;
							} break;
						}
						
						switch (vk_code) {
							case VK_RIGHT: {
								input.buttons[BUTTON_RIGHT].is_down = is_down;
								input.buttons[BUTTON_RIGHT].changed = true;
							} break;
						}
						
						switch (vk_code) {
							case VK_LEFT: {
								input.buttons[BUTTON_LEFT].is_down = is_down;
								input.buttons[BUTTON_LEFT].changed = true;
							} break;

						}

						switch (vk_code) {
							case VK_ESCAPE: {
								input.buttons[BUTTON_ESC].is_down = is_down;
								input.buttons[BUTTON_ESC].changed = true;
							} break;
						}

						switch (vk_code) {
							case VK_F1: {
								input.buttons[BUTTON_F1].is_down = is_down;
								input.buttons[BUTTON_F1].changed = true;
							} break;
						}

						switch (vk_code) {
						case VK_F2: {
							input.buttons[BUTTON_F2].is_down = is_down;
							input.buttons[BUTTON_F2].changed = true;
						} break;
						}

				} break;

				default: {
					TranslateMessage(&message);
					DispatchMessage(&message);
				}
			}
				
		}
		// Simulate
		
		simulate_game(&input, delta_time);
				
		// Render
		StretchDIBits(hdc, 0, 0, render_state.width, render_state.height, 0, 0, render_state.width, render_state.height, render_state.memory, &render_state.bitmap_info, DIB_RGB_COLORS, SRCCOPY);

		LARGE_INTEGER frame_end_time;
		QueryPerformanceCounter(&frame_end_time);
		delta_time = (float)(frame_end_time.QuadPart - frame_begin_time.QuadPart) / performance_frequency;
		frame_begin_time = frame_end_time;
	}

}

