#include "raylib.h"
#include <iostream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <queue>
#include <algorithm>
#include <cmath>

// ---------- Constants ----------
//Base Variables
const int Screen_Height = 600;
const int Screen_Width = 800;
const int Grid_Size = 20;
const int Rooms_Amount = 20;

const int GridColumns = Screen_Width / Grid_Size;
const int GridRows = Screen_Height / Grid_Size;

const int MaxInfluence = 10;

//Particle Variables
const int Particles_Amount = 2;
const int Particle_Life = 50;
const float Weight_Sharpness = 3.0f;   // each +1 of weight makes a cell this many times more likely
const float Step_Interval = 0.1f;     // seconds between particle moves
const float Base_Step_Cost = 1.0f;       // life lost per step on an empty cell
const float Path_Move_Discount = 0.2f;
const float Min_Step_Cost = 0.2f;

//Path Variables
const float Path_Initial_Bonus = 1.0f;   // attraction a freshly laid path starts with
const float Path_Weight_Gain = 1.0f;     // bonus added by the first reinforcement
const float Path_Weight_Falloff = 0.7f;  // each later reinforcement adds 70% of the previous one
const float Path_Cost_Gain = 0.15f;      // cost reduction from the first reinforcement
const float Path_Cost_Falloff = 0.5f;    // cost gains shrink faster than weight gains
const float Max_Path_Bonus = 4.0f;       // ceiling on a single cell's bonus
const float Path_Weight_Cap = MaxInfluence - 2.0f;  // a path cell's total weight never exceeds this
const float Path_Fade_Rate = 0.005f;        // bonus lost per iteration on a normal path
const float Path_Cost_Fade_Ratio = 0.2f;    // cost discount fades at this fraction of the bonus rate
const float Solid_Fade_Multiplier = 0.1f;   // solid paths fade at 10% of that speed. set to 0 for permanent solid paths

//Room Variables
const int Room_Discovery_Particles = 2;   // amount spawned the first time a room is found
const int Room_Revisit_Particles = 1;     // amount spawned if the room was already found

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
	bool solid = false;
	int roomId = -1;
};

struct Particle
{
	GridCoord position;
	std::vector<GridCoord> pathPoints;
	float life = 0.0f;
	int originRoomId = -1;
};

struct RoomRect
{
	int x = 0;
	int y = 0;
	int w = 0;
	int h = 0;
	int id = -1;
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

Color CellColor(const GridPoint& cell)
{
	if (cell.type != CellType::Path) return ColorFor(cell.type);

	float t = std::min(1.0f, cell.pathBonus / Path_Initial_Bonus);
	Color base = cell.solid ? SKYBLUE : DARKBLUE;
	return ColorAlpha(base, 0.2f + 0.8f * t);
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
		p.originRoomId = room.id;
		p.pathPoints.push_back(p.position);
		particles.push_back(p);
	}
	return particles;
}

int LabelRooms(std::vector<GridPoint>& grid)
{
	const GridCoord directions[4] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
	int nextId = 0;

	for (auto& start : grid)
	{
		if (start.type != CellType::Room || start.roomId != -1) continue; //check if already given an id

		std::queue<GridCoord> frontier;
		start.roomId = nextId;
		frontier.push(start.position);

		while (!frontier.empty())
		{
			GridCoord current = frontier.front();
			frontier.pop();

			for (const auto& d : directions)
			{
				int nx = current.x + d.x;
				int ny = current.y + d.y;

				if (nx < 0 || nx >= GridColumns || ny < 0 || ny >= GridRows) continue;

				GridPoint& neighbour = grid[IndexOf(nx, ny)];
				if (neighbour.type != CellType::Room || neighbour.roomId != -1) continue;

				neighbour.roomId = nextId;
				frontier.push({ nx, ny });
			}
		}
		nextId++;
	}
	return nextId;
}

std::vector<int> DistanceFromRoom(const std::vector<GridPoint>& grid, int roomId)
{
	const int Unvisited = -1;
	const GridCoord directions[4] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
	std::vector<int> distance(grid.size(), Unvisited);
	std::queue<GridCoord> frontier;

	for (const auto& cell : grid)
	{
		if (cell.roomId != roomId) continue;
		distance[IndexOf(cell.position.x, cell.position.y)] = 0;
		frontier.push(cell.position);
	}

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
			if (distance[index] != Unvisited) continue;

			distance[index] = currentDistance + 1;
			frontier.push({ nx, ny });
		}
	}
	return distance;
}

// ---------- Generation ----------
RoomRect GenerateDungeon(std::vector<GridPoint>& grid, std::vector<bool>& roomFound, std::vector<std::vector<int>>& roomDistance)
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

	int roomCount = LabelRooms(grid);
	int startRoomId = roomCount;

	for (int dx = 0; dx < startRoomGridWidth; dx++) // spawn starting room
	{
		for (int dy = 0; dy < startRoomGridHeight; dy++)
		{
			GridPoint& cell = grid[IndexOf(startRoomGridX + dx, startRoomGridY + dy)];
			cell.type = CellType::StartRoom;
			cell.roomId = startRoomId;
		}
	}
	roomFound.assign(roomCount, false);
	roomDistance.clear();
	for (int id = 0; id <= startRoomId; id++)
	{
		roomDistance.push_back(DistanceFromRoom(grid, id));
	}
	CalculateWeights(grid);
	return { startRoomGridX, startRoomGridY, startRoomGridWidth, startRoomGridHeight, startRoomId };
}

int PickWeightedIndex(const std::vector<float>& weights)
{
	float best = *std::max_element(weights.begin(), weights.end());

	std::vector<float> scores(weights.size());
	float total = 0.0f;
	for (size_t i = 0; i < weights.size(); i++)
	{
		scores[i] = std::pow(Weight_Sharpness, weights[i] - best);
		total += scores[i];
	}

	float roll = (std::rand() / static_cast<float>(RAND_MAX)) * total;

	for (size_t i = 0; i < scores.size(); i++)
	{
		roll -= scores[i];
		if (roll < 0.0f)
		{
			return static_cast<int>(i);
		}
	}
	return static_cast<int>(scores.size()) - 1;
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

void SolidifyPath(const Particle& p, std::vector<GridPoint>& grid)
{
	for (const auto& c : p.pathPoints)
	{
		GridPoint& cell = grid[IndexOf(c.x, c.y)];
		if (cell.type == CellType::Path) cell.solid = true;
	}
}

float EffectiveWeight(const GridPoint& cell, float baseWeight)
{
	float w = baseWeight;
	if (cell.type == CellType::Path)
	{
		w = std::max(w, std::min(w + cell.pathBonus, Path_Weight_Cap));
	}
	return w;
}

float RoomInfluence(const std::vector<std::vector<int>>& roomDistance, int cellIndex, int ignoreRoomId)
{
	int best = 0;
	for (size_t r = 0; r < roomDistance.size(); r++)
	{
		if (static_cast<int>(r) == ignoreRoomId) continue;

		int d = roomDistance[r][cellIndex];
		if (d < 0) continue;

		best = std::max(best, MaxInfluence - d);
	}
	return static_cast<float>(best);
}

std::vector<GridCoord> GetRoomIdRingCells(const std::vector<GridPoint>& grid, int roomId)
{
	const GridCoord directions[4] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
	std::vector<bool> seen(grid.size(), false);
	std::vector<GridCoord> ring;

	for (const auto& cell : grid)
	{
		if (cell.type != CellType::Room || cell.roomId != roomId) continue;

		for (const auto& d : directions)
		{
			int nx = cell.position.x + d.x;
			int ny = cell.position.y + d.y;

			if (nx < 0 || nx >= GridColumns || ny < 0 || ny >= GridRows) continue;

			int index = IndexOf(nx, ny);
			if (seen[index]) continue;

			CellType t = grid[index].type;
			if (t != CellType::Empty && t != CellType::Path) continue;

			seen[index] = true;
			ring.push_back({ nx, ny });
		}
	}
	return ring;
}

Particle MakeParticle(GridCoord position, int originRoomId)
{
	Particle p;
	p.position = position;
	p.life = Particle_Life;
	p.originRoomId = originRoomId;
	p.pathPoints.push_back(position);
	return p;
}

void SpawnFromRoom(const std::vector<GridPoint>& grid, int roomId, int count, std::vector<Particle>& out)
{
	std::vector<GridCoord> ring = GetRoomIdRingCells(grid, roomId);
	if (ring.empty()) return;

	for (int i = 0; i < count; i++)
	{
		out.push_back(MakeParticle(ring[std::rand() % ring.size()], roomId));
	}
}

void IterateParticles(std::vector<Particle>& particles, std::vector<GridPoint>& grid, std::vector<bool>& roomFound, const std::vector<std::vector<int>>& roomDistance)
{
	const GridCoord directions[4] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
	std::vector<Particle> newParticles;

	for (auto& p : particles)
	{
		std::vector<GridCoord> candidates;
		std::vector<float> candidateWeights;

		for (const auto& d : directions)
		{
			int nx = p.position.x + d.x;
			int ny = p.position.y + d.y;

			if (nx < 0 || nx >= GridColumns || ny < 0 || ny >= GridRows) continue;

			int index = IndexOf(nx, ny);
			const GridPoint& cell = grid[index];

			if (cell.type == CellType::StartRoom) continue;
			if (cell.roomId >= 0 && cell.roomId == p.originRoomId) continue; // can't re-enter its origin room
			if (PathContains(p.pathPoints, nx, ny)) continue;

			float base = RoomInfluence(roomDistance, index, p.originRoomId); // ignores the origin room's weighting
			candidates.push_back({ nx, ny });
			candidateWeights.push_back(EffectiveWeight(cell, base));
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
			SolidifyPath(p, grid); 
			int id = destCell.roomId;
			if (id >= 0 && id < static_cast<int>(roomFound.size()))
			{
				int count = roomFound[id] ? Room_Revisit_Particles : Room_Discovery_Particles;
				roomFound[id] = true;
				SpawnFromRoom(grid, id, count, newParticles);
			}
			p.life = 0;
		}
		else
		{
			p.life -= stepCost;
		}
	}

	// remove dead particles after the loop, never during it
	particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) { return p.life <= 0; }), particles.end());
	particles.insert(particles.end(), newParticles.begin(), newParticles.end());
}

void ProgressPaths(std::vector<GridPoint>& grid)
{
	for (auto& cell : grid)
	{
		if (cell.type != CellType::Path) continue;

		float fade = Path_Fade_Rate * (cell.solid ? Solid_Fade_Multiplier : 1.0f);

		cell.pathBonus = std::max(0.0f, cell.pathBonus - fade);
		cell.moveCost = std::min(0.0f, cell.moveCost + fade * Path_Cost_Fade_Ratio);

		if (cell.pathBonus <= 0.0f) // fully faded: back to a plain empty cell
		{
			cell.type = CellType::Empty;
			cell.moveCost = 0.0f;
			cell.pathUses = 0;
			cell.solid = false;
		}
	}
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

		DrawRectangle(px, py, Grid_Size, Grid_Size, CellColor(point));

		if (showWeights)
		{
			if (point.type == CellType::Path)
			{
				DrawText(TextFormat("%.1f", EffectiveWeight(point, static_cast<float>(point.weight))), px + 2, py + 5, 10, YELLOW);
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
	std::vector<bool> roomFound;
	std::vector<std::vector<int>> roomDistance;

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
	RoomRect startRoom = GenerateDungeon(gridPoints, roomFound, roomDistance);
	std::vector<Particle> particles = SpawnParticles(gridPoints, startRoom);

	// game loop
	while (!WindowShouldClose())
	{
		if (IsKeyPressed(KEY_R)) // regenerate
		{
			ClearGrid(gridPoints);
			startRoom = GenerateDungeon(gridPoints, roomFound, roomDistance);
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
			IterateParticles(particles, gridPoints, roomFound, roomDistance);
			ProgressPaths(gridPoints);
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