#include "raylib.h"
#include <iostream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <queue>
#include <algorithm>
#include <cmath>

// ---------- Constants ----------
const int Screen_Height = 600;
const int Screen_Width = 800;
const int Grid_Size = 20;
const int Rooms_Amount = 20;

const int GridColumns = Screen_Width / Grid_Size;
const int GridRows = Screen_Height / Grid_Size;

const int MaxInfluence = 10;

const int Particles_Amount = 5;
const int Particle_Life = 50;
const int Weight_Bias = 1;            // added to every weight so zero-weight cells can still be picked, bigger bias means more randomness in pathing
const float Step_Interval = 0.1f;     // seconds between particle moves
const float Base_Step_Cost = 1.0f;       // life lost per step on an empty cell
const float Path_Move_Discount = 0.2f;
const float Min_Step_Cost = 0.2f;

const float Path_Initial_Bonus = 1.0f;   // attraction a freshly laid path starts with
const float Path_Weight_Gain = 1.0f;     // bonus added by the first reinforcement
const float Path_Weight_Falloff = 0.7f;  // each later reinforcement adds 70% of the previous one
const float Path_Cost_Gain = 0.15f;      // cost reduction from the first reinforcement
const float Path_Cost_Falloff = 0.5f;    // cost gains shrink faster than weight gains
const float Max_Path_Bonus = 4.0f;       // ceiling on a single cell's bonus
const float Path_Weight_Cap = MaxInfluence - 2.0f;  // a path cell's total weight never exceeds this

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
	float moveCost = 0.0f;
	float pathBonus = 0.0f;
	int pathUses = 0;
};

struct Particle
{
	GridCoord position;
	std::vector<GridCoord> pathPoints;
	float life = 0.0f;
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

			bool outsideX = x < room.x || x >= room.x + room.w;
			bool outsideY = y < room.y || y >= room.y + room.h;
			if (outsideX && outsideY) continue; // checknig for corners

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

int PickWeightedIndex(const std::vector<float>& weights)
{
	float total = 0.0f;
	for (float w : weights)
	{
		total += w + Weight_Bias;
	}

	float roll = (std::rand() / static_cast<float>(RAND_MAX)) * total;

	for (size_t i = 0; i < weights.size(); i++)
	{
		roll -= weights[i] + Weight_Bias;
		if (roll < 0.0f)
		{
			return static_cast<int>(i);
		}
	}
	return static_cast<int>(weights.size()) - 1;
}

bool PathContains(const std::vector<GridCoord>& path, int x, int y)
{
	return std::any_of(path.begin(), path.end(),
		[x, y](const GridCoord& c) { return c.x == x && c.y == y; });
}

void ReinforcePath(GridPoint& cell)
{
	float uses = static_cast<float>(cell.pathUses);
	cell.pathBonus = std::min(Max_Path_Bonus, cell.pathBonus + Path_Weight_Gain * std::pow(Path_Weight_Falloff, uses));
	cell.moveCost = std::max(Min_Step_Cost - Base_Step_Cost, cell.moveCost - Path_Cost_Gain * std::pow(Path_Cost_Falloff, uses));
	cell.pathUses++;
}

float EffectiveWeight(const GridPoint& cell)
{
	float w = static_cast<float>(cell.weight);
	if (cell.type == CellType::Path)
	{
		w = std::max(w, std::min(w + cell.pathBonus, Path_Weight_Cap));
	}
	return w;
}

void IterateParticles(std::vector<Particle>& particles, std::vector<GridPoint>& grid)
{
	const GridCoord directions[4] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };

	for (auto& p : particles)
	{
		std::vector<GridCoord> candidates;
		std::vector<float> candidateWeights;

		for (const auto& d : directions)
		{
			int nx = p.position.x + d.x;
			int ny = p.position.y + d.y;

			if (nx < 0 || nx >= GridColumns || ny < 0 || ny >= GridRows) continue;

			const GridPoint& cell = grid[IndexOf(nx, ny)];
			if (cell.type == CellType::StartRoom) continue; // don't wander back into the start room
			if (PathContains(p.pathPoints, nx, ny)) continue; //don't double back on own path

			float w = static_cast<float>(cell.weight);

			if (cell.type == CellType::Path)
			{
				w = std::min(w + cell.pathBonus, Path_Weight_Cap);
			}
			candidates.push_back({ nx, ny });
			candidateWeights.push_back(EffectiveWeight(cell));
		}

		if (candidates.empty())
		{
			p.life = 0; // boxed in
			continue;
		}

		int choice = PickWeightedIndex(candidateWeights);

		GridCoord previous = p.position;
		p.position = candidates[choice];
		p.pathPoints.push_back(p.position);

		GridPoint& destCell = grid[IndexOf(p.position.x, p.position.y)];
		GridPoint& previousCell = grid[IndexOf(previous.x, previous.y)];

		// cost of the cell we're stepping INTO; paths laid by other particles make it cheaper
		float stepCost = std::max(Min_Step_Cost, Base_Step_Cost + destCell.moveCost);

		if (destCell.type == CellType::Path)
		{
			ReinforcePath(destCell);
		}

		// leave a trail on the cell we just left
		if (previousCell.type == CellType::Empty)
		{
			previousCell.type = CellType::Path;
			previousCell.moveCost -= Path_Move_Discount;
			previousCell.pathBonus = Path_Initial_Bonus;
			previousCell.pathUses = 0;
		}

		if (destCell.type == CellType::Room)
		{
			// TODO: solidify p.pathPoints, spawn new particles around this room
			p.life = 0;
		}
		else
		{
			p.life -= stepCost;
		}
	}

	// remove dead particles after the loop, never during it
	particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) { return p.life <= 0; }), particles.end());
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
			if (point.type == CellType::Path)
			{
				DrawText(TextFormat("%.1f", EffectiveWeight(point)), px + 2, py + 5, 10, YELLOW);
			}
			else
			{
				DrawText(TextFormat("%d", point.weight), px + 4, py + 4, 10, WHITE);
			}
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


// ---------- Main ----------
int main()
{
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(Screen_Width, Screen_Height, "Slime Generator");

	bool showWeights = false;
	float stepTimer = 0.0f;
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
	RoomRect startRoom = GenerateDungeon(gridPoints);
	std::vector<Particle> particles = SpawnParticles(gridPoints, startRoom);

	// game loop
	while (!WindowShouldClose())
	{
		if (IsKeyPressed(KEY_R)) // regenerate
		{
			ClearGrid(gridPoints);
			startRoom = GenerateDungeon(gridPoints);
			particles = SpawnParticles(gridPoints, startRoom);
		}
		if (IsKeyPressed(KEY_W))
		{
			showWeights = !showWeights;
		}
		stepTimer += GetFrameTime();
		if (stepTimer >= Step_Interval)
		{
			stepTimer = 0.0f;
			IterateParticles(particles, gridPoints);
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