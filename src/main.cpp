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
		Vector2 position = Vector2(0, 0);
		int weight = 0;
		bool isRoom = false;
		bool isPath = false;
		int pathLife = 0;
		int moveCost = 0;
		Color color = BLACK;
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

	auto indexOf = [&](int x, int y) { return x * GridRows + y; };
	
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(Screen_Width, Screen_Height, "Slime Generator");
	SearchAndSetResourceDir("resources");
	
	int startRoomGridWidth = (std::rand() % 4) + 2;
	int startRoomGridHeight = (std::rand() % 4) + 2;
	int startRoomGridX = std::rand() % (GridColumns - startRoomGridWidth + 1);
	int startRoomGridY = std::rand() % (GridRows - startRoomGridHeight + 1);

	for (int i = 0; i < Rooms_Amount; i++) //spawn the rooms
	{
		int roomGridWidth = (std::rand() % 4) + 2;
		int roomGridHeight = (std::rand() % 4) + 2;

		int gridX = std::rand() % (GridColumns - roomGridWidth + 1);
		int gridY = std::rand() % (GridRows - roomGridHeight + 1);
		
		for (int dx = 0; dx < roomGridWidth; dx++)
		{
			for (int dy = 0; dy < roomGridHeight; dy++)
			{
				GridPoint& point = gridPoints[indexOf(gridX + dx, gridY + dy)];
				point.isRoom = true;
				point.color = RED;
			}
		}
	}

	for (int dx = 0; dx < startRoomGridWidth; dx++) //spawn starting room
	{
		for (int dy = 0; dy < startRoomGridHeight; dy++)
		{
			GridPoint& point = gridPoints[indexOf(startRoomGridX + dx, startRoomGridY + dy)];
			point.isRoom = true;
			point.color = PINK;
		}
	}
	
	// game loop
	while (!WindowShouldClose())		// run the loop until the user presses ESCAPE or presses the Close button on the window
	{
		// drawing
		BeginDrawing();

		// Setup the back buffer for drawing (clear color and depth buffers)
		ClearBackground(BLACK);

		for (const auto& point : gridPoints) //new way for actually drawing based on grid cells
		{
			DrawRectangle(point.position.x * Grid_Size,
				point.position.y * Grid_Size,
				Grid_Size, Grid_Size, point.color);
		}

		for (int i = 0; i < Screen_Width; i += Grid_Size) {
			DrawLine(i, 0, i, Screen_Height, LIGHTGRAY);
		}
		for (int i = 0; i < Screen_Height; i += Grid_Size) {
			DrawLine(0, i, Screen_Width, i, LIGHTGRAY);
		}
		
		// end the frame and get ready for the next one  (display frame, poll input, etc...)
		EndDrawing();
	}

	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}
