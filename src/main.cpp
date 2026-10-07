#include "raylib.h"
#include <iostream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <queue>
#include <algorithm>

// ---------- Constants ----------
const int Screen_Height = 600;
const int Screen_Width = 800;
const int Grid_Size = 20;
const int Rooms_Amount = 20;

const int GridColumns = Screen_Width / Grid_Size;
const int GridRows = Screen_Height / Grid_Size;

const int MaxInfluence = 5;

// ---------- Types ----------
enum class CellType
{
	Empty,
	Room,
	StartRoom,
	Path
};

struct GridCoord
{
	int x = 0;
	int y = 0;
};

struct GridPoint
{
	GridCoord position;
	CellType type = CellType::Empty;
	int weight = 0;
	int pathLife = 0;
	int moveCost = 0;
};

struct Particle
{
	GridCoord position;
	std::vector<GridCoord> pathPoints;
	int life = 0;
};

// ---------- Grid helpers ----------
int IndexOf(int x, int y)
{
	return x * GridRows + y;
}

Color ColorFor(CellType type)
{
	switch (type)
	{
	case CellType::Room:      return RED;
	case CellType::StartRoom: return PINK;
	case CellType::Path:      return DARKBLUE;
	default:                  return BLACK;
	}
}

std::vector<GridPoint> CreateGrid()
{
	std::vector<GridPoint> grid;
	grid.reserve(GridColumns * GridRows);

	for (int x = 0; x < GridColumns; x++)
	{
		for (int y = 0; y < GridRows; y++)
		{
			GridPoint newPoint;
			newPoint.position = { x, y };
			grid.push_back(newPoint);
		}
	}
	return grid;
}

void ClearGrid(std::vector<GridPoint>& grid)
{
	for (auto& point : grid)
	{
		GridCoord pos = point.position;
		point = GridPoint();
		point.position = pos;
	}
}

// ---------- Generation ----------
void GenerateDungeon(std::vector<GridPoint>& grid)
{
	int startRoomGridWidth = (std::rand() % 4) + 2;
	int startRoomGridHeight = (std::rand() % 4) + 2;
	int startRoomGridX = std::rand() % (GridColumns - startRoomGridWidth + 1);
	int startRoomGridY = std::rand() % (GridRows - startRoomGridHeight + 1);

	for (int i = 0; i < Rooms_Amount; i++) // spawn the rooms
	{
		int roomGridWidth = (std::rand() % 4) + 2;
		int roomGridHeight = (std::rand() % 4) + 2;

		int gridX = std::rand() % (GridColumns - roomGridWidth + 1);
		int gridY = std::rand() % (GridRows - roomGridHeight + 1);

		for (int dx = 0; dx < roomGridWidth; dx++)
		{
			for (int dy = 0; dy < roomGridHeight; dy++)
			{
				grid[IndexOf(gridX + dx, gridY + dy)].type = CellType::Room;
			}
		}
	}

	for (int dx = 0; dx < startRoomGridWidth; dx++) // spawn starting room
	{
		for (int dy = 0; dy < startRoomGridHeight; dy++)
		{
			grid[IndexOf(startRoomGridX + dx, startRoomGridY + dy)].type = CellType::StartRoom;
		}
	}
	//CalculateWeights();
}

void CalculateWeights(std::vector<GridPoint>& grid)
{
	const int Unvisited = -1;
	std::vector<int> distance(grid.size(), Unvisited);
	std::queue<GridCoord> frontier;

	for (const auto& point : grid)
	{
		if (point.type == CellType::Room || point.type == CellType::StartRoom)
		{
			distance[IndexOf(point.position.x, point.position.y)] = 0;
			frontier.push(point.position);
		}
	}
	
	const GridCoord directions[4] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };

	while (!frontier.empty())
	{
		GridCoord current = frontier.front();
		frontier.pop();
		int currentDistance = distance[IndexOf(current.x, current.y)];

		for (const auto& d : directions)
		{
			int nx = current.x + d.x;
			int ny = current.y + d.y;

			if (nx < 0 || nx >= GridColumns || ny < 0 || ny >= GridRows) continue;

			int index = IndexOf(nx, ny);
			if (distance[index] != Unvisited) continue; // already reached by a closer room

			distance[index] = currentDistance + 1;
			frontier.push({ nx, ny });
		}
	}

	for (size_t i = 0; i < grid.size(); i++)
	{
		//grid[i].weight = distance[i];
		grid[i].weight = std::max(0, MaxInfluence - distance[i]);
	}
}

// ---------- Drawing ----------
void DrawGrid(const std::vector<GridPoint>& grid, bool showWeights)
{
	for (const auto& point : grid)
	{
		int px = point.position.x * Grid_Size;
		int py = point.position.y * Grid_Size;

		DrawRectangle(px, py, Grid_Size, Grid_Size, ColorFor(point.type));
		DrawRectangle(point.position.x * Grid_Size, point.position.y * Grid_Size, Grid_Size, Grid_Size, ColorFor(point.type));

		if (showWeights)
		{
			DrawText(TextFormat("%d", point.weight), px + 4, py + 4, 10, WHITE);
		}
	}

	for (int i = 0; i < Screen_Width; i += Grid_Size)
	{
		DrawLine(i, 0, i, Screen_Height, LIGHTGRAY);
	}
	for (int i = 0; i < Screen_Height; i += Grid_Size)
	{
		DrawLine(0, i, Screen_Width, i, LIGHTGRAY);
	}
}

// ---------- Main ----------
int main()
{
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(Screen_Width, Screen_Height, "Slime Generator");

	bool showWeights = false;
	int seed = 0;

	if (seed == 0)
	{
		srand(static_cast<unsigned int>(time(NULL)));
		seed = time(NULL);
	}
	else srand(seed);

	std::cout << "Grid Columns:" << GridColumns << "\n";
	std::cout << "Grid Rows:" << GridRows << "\n";
	std::cout << "Seed:" << seed << "\n";

	std::vector<GridPoint> gridPoints = CreateGrid();
	GenerateDungeon(gridPoints);

	// game loop
	while (!WindowShouldClose())
	{
		if (IsKeyPressed(KEY_R)) // regenerate
		{
			ClearGrid(gridPoints);
			GenerateDungeon(gridPoints);
		}
		if (IsKeyPressed(KEY_W))
		{
			showWeights = !showWeights;
			//DrawGrid(gridPoints, showWeights);
		}

		BeginDrawing();
		ClearBackground(BLACK);
		DrawGrid(gridPoints, showWeights);
		EndDrawing();
	}

	CloseWindow();
	return 0;
}