#pragma once
#include "raylib.h"

namespace PCG {
    // Tile Types (Using Enum for readability)
    typedef enum {
        TILE_TYPE_GRASS = 0,
        TILE_TYPE_ROCK = 1,
        TILE_COUNT  // Automatically counts total types
    } TileType;

    // Visual & Character settings
    constexpr char GRASS_CHAR = '.';
    constexpr char ROCK_CHAR = '#';
    constexpr Color GRASS_COLOR = { 69, 182, 156, 255 };
    constexpr Color ROCK_COLOR = { 114, 147, 160, 255 };
    constexpr Color UNKNOWN_COLOR = WHITE;

    // File Names
    constexpr char* MAP_TEXT_FILENAME = "pcg_map_data.txt";
    constexpr char* MAP_IMAGE_FILENAME = "pcg_map.png";

    // Screen & Map Dimensions
    constexpr int SCREEN_WIDTH = 1024;
    constexpr int SCREEN_HEIGHT = 1024;
    constexpr int TILE_SIZE = 4;
    constexpr int MAP_COLUMNS = (SCREEN_WIDTH / TILE_SIZE);
    constexpr int MAP_ROWS = (SCREEN_HEIGHT / TILE_SIZE);

    // UI variable defines used to position buttons on screen
    constexpr int BUTTON_WIDTH = 200;
    constexpr int BUTTON_HEIGHT = 50;
    constexpr int BUTTON_X = (SCREEN_WIDTH - BUTTON_WIDTH - 20);
    constexpr int BUTTON_Y = (SCREEN_HEIGHT - BUTTON_HEIGHT - 20);
    constexpr Rectangle RESET_BUTTON_BOUNDS = { BUTTON_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT };

    // Pure Virtual Base Class for Map Generation Algorithms
    class MapGenerator {
    public:
        virtual ~MapGenerator() = default; // virtual destructor
        // This enforces that every child class MUST write their own Generate function.
        virtual void Generate(TileType _tileArray[MAP_ROWS][MAP_COLUMNS]) = 0;
    };

    // Derived classes for different map generation algorithms can be defined here.
    // Random Map Generator
    class RandomMapGenerator : public MapGenerator {
    public:
        RandomMapGenerator();   // constructor
        ~RandomMapGenerator();  // destructor
        void Generate(TileType _tileArray[MAP_ROWS][MAP_COLUMNS]) override; // Override the pure virtual function
    };

    // Noise Map Generator (Perlin Noise or Simplex Noise could be used here)
    class NoiseMapGenerator : public MapGenerator {
    public:
        NoiseMapGenerator();   // constructor
        ~NoiseMapGenerator();  // destructor
        void Generate(TileType _tileArray[MAP_ROWS][MAP_COLUMNS]) override; // Override the pure virtual function
    };

    class TileMap {
    public:
        TileMap();  // constructor
        ~TileMap(); // destructor

        // Core Actions
        // Function Declarations
        void DrawMap() const; // 'const' means this functino won't change the map data
        void PrintMap() const;
        void DrawGUI();

        // I/O Functions
        void SaveMapData(const char* filename) const;
        void SaveMapImage(const char* filename) const;
        void LoadMapData(const char* filename);

        // Accessors (Getters/Setters)
        void SetTile(int x, int y, PCG::TileType tileType);
        Color GetTileColor(TileType tileType) const;
        char GetTileChar(TileType tileType) const;

        TileType(&GetTileData())[MAP_ROWS][MAP_COLUMNS]{
            return tileArray;
        }
            // getter /setter for map generator 
        void SetMapGenerator(MapGenerator* generator);
        MapGenerator* GetMapGenerator() const;

    private:
        TileType tileArray[MAP_ROWS][MAP_COLUMNS] = { PCG::TileType::TILE_TYPE_ROCK };  // 2D array to hold tile types for the map
        MapGenerator* mapGenerator;
    };


}