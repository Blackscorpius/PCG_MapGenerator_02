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

const int MaxInfluence = 10;

const int Particles_Amount = 5;
const int Particle_Life = 10;

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

struct RoomRect
{
	int x = 0;
	int y = 0;
	int w = 0;
	int h = 0;
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
	grid.reserve(GridColumns * GridRows); //reserves this memory for all the cells upfront instead of trying to expand allocation later

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

void ClearGrid(std::vector<GridPoint>& grid) //clears grid 
{
	for (auto& point : grid)
	{
		GridCoord pos = point.position;
		point = GridPoint();
		point.position = pos;
	}
}

void CalculateWeights(std::vector<GridPoint>& grid) //using BFS to expand out from rooms and generate weight fields
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
		grid[i].weight = std::max(0, MaxInfluence - distance[i]); //inverts weight so closer rooms have heigher weight
	}
}

bool RoomsOverlap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh)
{
	return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

std::vector<GridCoord> GetRoomRingCells(const std::vector<GridPoint>& grid, const RoomRect& room)
{
	std::vector<GridCoord> ring;

	for (int x = room.x - 1; x <= room.x + room.w; x++)
	{
		for (int y = room.y - 1; y <= room.y + room.h; y++)
		{
			bool insideRoom = x >= room.x && x < room.x + room.w &&
				y >= room.y && y < room.y + room.h;
			if (insideRoom) continue;

			if (x < 0 || x >= GridColumns || y < 0 || y >= GridRows) continue;

			if (grid[IndexOf(x, y)].type != CellType::Empty) continue;

			ring.push_back({ x, y });
		}
	}
	return ring;
}

std::vector<Particle> SpawnParticles(const std::vector<GridPoint>& grid, const RoomRect& room)
{
	std::vector<GridCoord> ring = GetRoomRingCells(grid, room);
	std::vector<Particle> particles;

	if (ring.empty()) return particles;

	particles.reserve(Particles_Amount);

	for (int i = 0; i < Particles_Amount; i++)
	{
		Particle p;
		p.position = ring[std::rand() % ring.size()];
		p.life = Particle_Life;
		p.pathPoints.push_back(p.position);
		particles.push_back(p);
	}
	return particles;
}

// ---------- Generation ----------
RoomRect GenerateDungeon(std::vector<GridPoint>& grid)
{
	int startRoomGridWidth = (std::rand() % 4) + 2;
	int startRoomGridHeight = (std::rand() % 4) + 2;
	int startRoomGridX = std::rand() % (GridColumns - startRoomGridWidth + 1);
	int startRoomGridY = std::rand() % (GridRows - startRoomGridHeight + 1);

	const int Max_Placement_Attempts = 20;

	for (int i = 0; i < Rooms_Amount; i++) // spawn the rooms
	{
		int roomGridWidth = (std::rand() % 4) + 2;
		int roomGridHeight = (std::rand() % 4) + 2;

		int gridX = 0;
		int gridY = 0;
		bool overlapsStart = true;

		for (int attempt = 0; attempt < Max_Placement_Attempts && overlapsStart; attempt++)
		{
			gridX = std::rand() % (GridColumns - roomGridWidth + 1);
			gridY = std::rand() % (GridRows - roomGridHeight + 1);

			overlapsStart = RoomsOverlap(gridX, gridY, roomGridWidth, roomGridHeight, startRoomGridX, startRoomGridY, startRoomGridWidth, startRoomGridHeight);
		}

		if (overlapsStart) continue; // couldn't find a valid spot, skip this room

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
	CalculateWeights(grid);
	return { startRoomGridX, startRoomGridY, startRoomGridWidth, startRoomGridHeight };
}


// ---------- Drawing ----------
void DrawGrid(const std::vector<GridPoint>& grid, bool showWeights)
{
	for (const auto& point : grid)
	{
		int px = point.position.x * Grid_Size;
		int py = point.position.y * Grid_Size;

		DrawRectangle(px, py, Grid_Size, Grid_Size, ColorFor(point.type));

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

void DrawParticles(const std::vector<Particle>& particles)
{
	for (const auto& p : particles)
	{
		DrawRectangle(p.position.x * Grid_Size + 5,
			p.position.y * Grid_Size + 5,
			Grid_Size - 10, Grid_Size - 10, YELLOW);
	}
}

void IterateParticles(const std::vector<Particle>& particles, const std::vector<GridPoint>& options)
{
	for (const auto& p : particles)
	{
		for (int x = p.position.x - 1; x <= p.position.x; x++)
		{
			for (int y = p.position.y -1; y <= p.position.y; y++)
			{
				GridCoord gridPos;
				gridPos.x = x;
				gridPos.y = y;
				//get gridpoint with grid coord of this value
				auto op = std::find_if(options.begin(), options.end(), [gridPos](const std::vector<GridPoint>& s)
					{
						return 0;
					});
				//check what its weight is or if room tile
				//store weight of tile
			}
		}
		//randomly select a tile to move to based loosely on the weights (can still choose imperfect option)
		//set current tile as path
		//set new position based on selected tile
		//add previous tile to path - increase weighting of path tile and energy efficiency of path (diminishing returns on both, but especially energy efficiency)
		//check if new tile is room - if is, solidify all tiles from path and trigger particle spawns around collided room
		// then destroy particle
		// otherwisee
		//decrease particle lifetime, decrease by less if moving on a path tile - kill if out of life
		//
	}
}

void ProgressPaths()
{
	//for each path
	//slightly fade extra weighting and energy bonuses - fade much less if path is solidified
	//don't decrease past what the tile was before becoming a path
	//
}

void FinaliseGeneration()
{
	//after time (and/or all rooms connected), stop particles from spawning and remove all paths below a certain efficiency (maybe below half the efficienct they're given when becoming a path?)
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
	RoomRect startRoom = GenerateDungeon(gridPoints);
	std::vector<Particle> particles = SpawnParticles(gridPoints, startRoom);

	// game loop
	while (!WindowShouldClose())
	{
		if (IsKeyPressed(KEY_R)) // regenerate
		{
			ClearGrid(gridPoints);
			//GenerateDungeon(gridPoints);
			startRoom = GenerateDungeon(gridPoints);
			particles = SpawnParticles(gridPoints, startRoom);
		}
		if (IsKeyPressed(KEY_W))
		{
			showWeights = !showWeights;
		}

		BeginDrawing();
		ClearBackground(BLACK);
		DrawGrid(gridPoints, showWeights);
		DrawParticles(particles);
		EndDrawing();
	}

	CloseWindow();
	return 0;
}