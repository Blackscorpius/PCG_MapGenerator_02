/*
Raylib example file.
This is an example main file for a simple raylib project.
Use this as a starting point or replace it with your code.

by Jeffery Myers is marked with CC0 1.0. To view a copy of this license, visit https://creativecommons.org/publicdomain/zero/1.0/

*/

#include "raylib.h"
#include "resource_dir.h"	// utility header for SearchAndSetResourceDir
#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <cstdlib>

int main ()
{

	const int Screen_Height = 600;
	const int Screen_Width = 800;
	const int Grid_Size = 20;
	const int Rooms_Amount = 20;
	const int Salt_Amount = 5;

	std::vector<Rectangle> rooms;
	std::vector<Rectangle> salts;

	int seed = 0;

	if (seed == 0)
	{
		//SetRandomSeed((unsigned int)time(NULL));
		srand(static_cast<unsigned int>(time(NULL)));
		seed = time(NULL);
	}
	else
	{
		srand(seed);
	}

	int GridColumns = Screen_Width / Grid_Size;
	int GridRows = Screen_Height / Grid_Size; 
	
	std::cout << "Grid Columns:" << GridColumns << "\n";
	std::cout << "Grid Rows:" << GridRows << "\n";
	std::cout << "Seed:" << seed << "\n";
	
	
	// Tell the window to use vsync and work on high DPI displays
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);

	// Create the window and OpenGL context
	InitWindow(Screen_Width, Screen_Height, "Slime Generator");

	// Utility function from resource_dir.h to find the resources folder and set it as the current working directory so we can load from it
	SearchAndSetResourceDir("resources");
	
		for (int i = 0; i < Rooms_Amount; i++)
		{
			int roomGridWidth = (std::rand() % 4) + 2;
			int roomGridHeight = (std::rand() % 4) + 2;

			int gridX = std::rand() % (GridColumns - roomGridWidth + 1);
			int gridY = std::rand() % (GridRows - roomGridHeight + 1);

			/*if (IsKeyPressed(KEY_SPACE))
			{*/
				Rectangle newRoom;
				newRoom.x = gridX * Grid_Size;
				newRoom.y = gridY * Grid_Size;
				newRoom.width = roomGridWidth * Grid_Size;
				newRoom.height = roomGridHeight * Grid_Size;

				rooms.push_back(newRoom);

			//}
		}

		/*for (int i = 0; i < Salt_Amount; i++)
		{
			int saltgridX = std::rand() % (GridColumns);
			int saltgridY = std::rand() % (GridRows);
			Rectangle newSalt;
			newSalt.x = saltgridX * Grid_Size;
			newSalt.y = saltgridY * Grid_Size;

			rooms.push_back(newSalt);
		}*/
	
	// game loop
	while (!WindowShouldClose())		// run the loop until the user presses ESCAPE or presses the Close button on the window
	{

		// drawing
		BeginDrawing();

		// Setup the back buffer for drawing (clear color and depth buffers)
		ClearBackground(BLACK);

		for (int i = 0; i < Screen_Width; i += Grid_Size) {
			DrawLine(i, 0, i, Screen_Height, LIGHTGRAY);
		}
		for (int i = 0; i < Screen_Height; i += Grid_Size) {
			DrawLine(0, i, Screen_Width, i, LIGHTGRAY);
		}

		for (const auto& rect : rooms)
		{
			DrawRectangleRec(rect, MAROON);
			DrawRectangleLinesEx(rect, 2, WHITE);
		}
		/*for (const auto& recta : salts)
		{
			DrawRectangleRec(recta, WHITE);
			DrawRectangleLinesEx(recta, 2, WHITE);
		}*/
		
		// end the frame and get ready for the next one  (display frame, poll input, etc...)
		EndDrawing();
	}

	

	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}
