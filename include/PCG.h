#pragma once
#include "raylib.h"

namespace PCG {
    // Screen & Map Dimensions
    constexpr int SCREEN_WIDTH = 1024;
    constexpr int SCREEN_HEIGHT = 1024;
    constexpr int TILE_SIZE = 16;
    constexpr int MAP_COLUMNS = (SCREEN_WIDTH / TILE_SIZE);
    constexpr int MAP_ROWS = (SCREEN_HEIGHT / TILE_SIZE);
    constexpr Vector3 MAP_SIZE = { 32, 8, 32 };

    // UI variable defines
    constexpr int BUTTON_WIDTH = 200;
    constexpr int BUTTON_HEIGHT = 50;
    constexpr int BUTTON_X = (SCREEN_WIDTH - BUTTON_WIDTH - 20);
    constexpr int BUTTON_Y = (SCREEN_HEIGHT - BUTTON_HEIGHT - 20);
    constexpr Rectangle RESET_BUTTON_BOUNDS = { BUTTON_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT };

    // File Names (fixed constexpr type)
    constexpr const char* HEIGHTMAP_IMAGE_FILENAME = "heightmap.png";
    constexpr const char* HEIGHTMAP_DATA_FILENAME = "heightmap_data.txt";

    // Terrain Manager - handles all terrain generation, rendering, and saving
    class TerrainManager {
    public:
        TerrainManager();
        ~TerrainManager();

        // Terrain Generation
        void GenerateFromPerlinNoise();
        void GenerateFromCellularNoise();
        void GenerateRandomTerrain();

        // Rendering
        void Draw(const Camera& camera) const;
        Texture2D GetTexture() const { return heightmapTexture; }
        const Image& GetHeightmapImage() const { return heightmapImage; }

        // File I/O
        void SaveHeightmapImage(const char* filename) const;
        void SaveHeightmapData(const char* filename) const;
        bool LoadHeightmap(const char* filename);

        // Terrain manipulation
        void ApplyContrast(float contrast);
        void FillValleys(unsigned char maxHeight, unsigned char fillHeight);
        void CapPeaks(unsigned char minHeight, unsigned char capHeight);
        void ApplyFillAndCap(int fillLevel, int capLevel);

    private:
        Model terrainModel;
        Texture2D heightmapTexture;
        Image heightmapImage;
        Image baseImage;
        Vector3 mapPosition;
        bool isLoaded;

        // Helper methods
        void CleanupTerrain();
        void BuildTerrainFromImage();
        void RebuildModel();   // Rebuild model/texture without freeing the image
    };

    // GUI Function Declaration
    void DrawGUI(TerrainManager& terrainManager);
}