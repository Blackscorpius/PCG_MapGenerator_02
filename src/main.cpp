/*
Raylib example file.
This is an example main file for a simple raylib project.
Use this as a starting point or replace it with your code.

by Jeffery Myers is marked with CC0 1.0. To view a copy of this license, visit https://creativecommons.org/publicdomain/zero/1.0/

*/

#include "raylib.h"
#include "resource_dir.h"
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

	std::vector<Rectangle> rooms;
	std::vector<Rectangle> startRoom;

	struct GridPoint
	{
		Vector2 position;
		int weight;
		bool isRoom;
		bool isPath;
		int pathLife;
		int moveCost;
	};
	struct Particle
	{
		Vector2 position;
		std::vector<GridPoint> pathPoints;
		int Life;
	};

	std::vector<GridPoint> gridPoints;

	int seed = 0;

	if (seed == 0)
	{
		srand(static_cast<unsigned int>(time(NULL)));
		seed = time(NULL);
	}
	else srand(seed);



	int GridColumns = Screen_Width / Grid_Size;
	int GridRows = Screen_Height / Grid_Size; 

	for (int x = 0; x < GridColumns; x++)
	{
		for (int y = 0; y < GridRows; y++)
		{
			GridPoint newPoint;
			newPoint.position = Vector2(x, y);
			gridPoints.push_back(newPoint);
		}
	}
	
	std::cout << "Grid Columns:" << GridColumns << "\n";
	std::cout << "Grid Rows:" << GridRows << "\n";
	std::cout << "Seed:" << seed << "\n";
	
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(Screen_Width, Screen_Height, "Slime Generator");
	SearchAndSetResourceDir("resources");
	
	int startRoomGridWidth = (std::rand() % 4) + 2;
	int startRoomGridHeight = (std::rand() % 4) + 2;
	int startRoomGridX = std::rand() % (GridColumns - startRoomGridWidth + 1);
	int startRoomGridY = std::rand() % (GridRows - startRoomGridHeight + 1);

	Rectangle newStartRoom; //spawn start room
	newStartRoom.x = startRoomGridX * Grid_Size;
	newStartRoom.y = startRoomGridY * Grid_Size;
	newStartRoom.width = startRoomGridWidth * Grid_Size;
	newStartRoom.height = startRoomGridHeight * Grid_Size;
	startRoom.push_back(newStartRoom);

	for (int i = 0; i < Rooms_Amount; i++) //spawn the rooms
	{
		int roomGridWidth = (std::rand() % 4) + 2;
		int roomGridHeight = (std::rand() % 4) + 2;

		int gridX = std::rand() % (GridColumns - roomGridWidth + 1);
		int gridY = std::rand() % (GridRows - roomGridHeight + 1);
		Vector2 posOnGrid = Vector2(gridX, gridY);

		/*Rectangle newRoom;
		newRoom.x = gridX * Grid_Size;
		newRoom.y = gridY * Grid_Size;
		newRoom.width = roomGridWidth * Grid_Size;
		newRoom.height = roomGridHeight * Grid_Size;

		rooms.push_back(newRoom);*/
		
		for (const auto& point : gridPoints)
		{
			if (point.position == posOnGrid)
			{

			}
		}


	}
	
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
		for (const auto& rect : startRoom)
		{
			DrawRectangleRec(rect, PINK);
			DrawRectangleLinesEx(rect, 2, WHITE);
		}
		
		// end the frame and get ready for the next one  (display frame, poll input, etc...)
		EndDrawing();
	}

	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}
