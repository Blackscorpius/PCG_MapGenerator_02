#include "PCG.h"
#include <iostream>
#include <fstream>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// =============================================
// TerrainManager Implementation
// =============================================

PCG::TerrainManager::TerrainManager() : isLoaded(false) {
    mapPosition = { (MAP_SIZE.x * 0.5f) * -1, 0, (MAP_SIZE.z * 0.5f) * -1 };
    heightmapImage = { 0 };
    heightmapTexture = { 0 };
    terrainModel = { 0 };
}

PCG::TerrainManager::~TerrainManager() {
    CleanupTerrain();
}

void PCG::TerrainManager::CleanupTerrain() {
    if (isLoaded) {
        UnloadTexture(heightmapTexture);
        UnloadModel(terrainModel);
        UnloadImage(heightmapImage);
        if (baseImage.data != NULL)
        {
            UnloadImage(baseImage);
            baseImage = { 0 };
        }
        isLoaded = false;
    }
}

void PCG::TerrainManager::BuildTerrainFromImage() {
    heightmapTexture = LoadTextureFromImage(heightmapImage);
    Mesh terrainMesh = GenMeshHeightmap(heightmapImage, MAP_SIZE);
    terrainModel = LoadModelFromMesh(terrainMesh);
    terrainModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = heightmapTexture;
    isLoaded = true;
}

void PCG::TerrainManager::RebuildModel() {
    UnloadTexture(heightmapTexture);
    UnloadModel(terrainModel);
    heightmapTexture = LoadTextureFromImage(heightmapImage);
    Mesh mesh = GenMeshHeightmap(heightmapImage, MAP_SIZE);
    terrainModel = LoadModelFromMesh(mesh);
    terrainModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = heightmapTexture;
}

// =============================================
// Terrain Generation Methods
// =============================================

void PCG::TerrainManager::GenerateFromPerlinNoise() {
    CleanupTerrain();

    int offsetX = GetRandomValue(0, 1000);
    int offsetY = GetRandomValue(0, 1000);
    float scale = 2.0f;

    heightmapImage = GenImagePerlinNoise(MAP_COLUMNS, MAP_ROWS, offsetX, offsetY, scale);
    
    baseImage = ImageCopy(heightmapImage);
    BuildTerrainFromImage();
    std::cout << "Perlin noise terrain generated!" << std::endl;
}

void PCG::TerrainManager::GenerateFromCellularNoise() {
    CleanupTerrain();

    heightmapImage = GenImageCellular(MAP_COLUMNS, MAP_ROWS, TILE_SIZE);

    baseImage = ImageCopy(heightmapImage);
    BuildTerrainFromImage();
    std::cout << "Cellular noise terrain generated!" << std::endl;
}

void PCG::TerrainManager::GenerateRandomTerrain() {
    CleanupTerrain();

    heightmapImage = GenImageColor(MAP_COLUMNS, MAP_ROWS, BLACK);

    for (int y = 0; y < MAP_ROWS; y++) {
        for (int x = 0; x < MAP_COLUMNS; x++) {
            unsigned char height = GetRandomValue(50, 200);
            Color pixelColor = { height, height, height, 255 };
            ImageDrawPixel(&heightmapImage, x, y, pixelColor);
        }
    }

    ImageBlurGaussian(&heightmapImage, 2);

    baseImage = ImageCopy(heightmapImage);
    BuildTerrainFromImage();
    std::cout << "Random terrain generated!" << std::endl;
}

// =============================================
// Terrain Manipulation
// =============================================

void PCG::TerrainManager::ApplyContrast(float contrast) {
    if (!isLoaded) return;
    ImageColorContrast(&heightmapImage, contrast);
    RebuildModel();
}

void PCG::TerrainManager::FillValleys(unsigned char maxHeight, unsigned char fillHeight) {
    if (!isLoaded) return;

    Color fillColor = { fillHeight, fillHeight, fillHeight, 255 };
    for (int i = 0; i < maxHeight; i++) {
        Color col = { (unsigned char)i, (unsigned char)i, (unsigned char)i, 255 };
        ImageColorReplace(&heightmapImage, col, fillColor);
    }

    RebuildModel();
}

void PCG::TerrainManager::CapPeaks(unsigned char minHeight, unsigned char capHeight) {
    if (!isLoaded) return;

    Color capColor = { capHeight, capHeight, capHeight, 255 };
    for (int i = 255; i > minHeight; i--) {
        Color col = { (unsigned char)i, (unsigned char)i, (unsigned char)i, 255 };
        ImageColorReplace(&heightmapImage, col, capColor);
    }

    RebuildModel();
}

void PCG::TerrainManager::ApplyFillAndCap(int fillLevel, int capLevel) {
    if (!isLoaded || baseImage.data == NULL) return;

    // Replace working image with a fresh copy of the original
    UnloadImage(heightmapImage);
    heightmapImage = ImageCopy(baseImage);

    // Fill valleys
    Color fillColor = { (unsigned char)fillLevel, (unsigned char)fillLevel, (unsigned char)fillLevel, 255 };
    for (int i = 0; i < fillLevel; i++) {
        Color col = { (unsigned char)i, (unsigned char)i, (unsigned char)i, 255 };
        ImageColorReplace(&heightmapImage, col, fillColor);
    }

    // Cap peaks
    Color capColor = { (unsigned char)capLevel, (unsigned char)capLevel, (unsigned char)capLevel, 255 };
    for (int i = 255; i > capLevel; i--) {
        Color col = { (unsigned char)i, (unsigned char)i, (unsigned char)i, 255 };
        ImageColorReplace(&heightmapImage, col, capColor);
    }

    RebuildModel();
}

// =============================================
// Rendering
// =============================================

void PCG::TerrainManager::Draw(const Camera& camera) const {
    if (isLoaded) {
        DrawModel(terrainModel, mapPosition, 1.0f, GREEN);
    }
    DrawGrid(50, 1.0f);
}

// =============================================
// File I/O
// =============================================

void PCG::TerrainManager::SaveHeightmapImage(const char* filename) const {
    if (!isLoaded) {
        std::cout << "No terrain to save!" << std::endl;
        return;
    }

    if (ExportImage(heightmapImage, filename)) {
        std::cout << "Heightmap image saved to: " << filename << std::endl;
    }
    else {
        std::cout << "Failed to save heightmap image!" << std::endl;
    }
}

void PCG::TerrainManager::SaveHeightmapData(const char* filename) const {
    if (!isLoaded) {
        std::cout << "No terrain data to save!" << std::endl;
        return;
    }

    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cout << "Failed to open file: " << filename << std::endl;
        return;
    }

    file << MAP_COLUMNS << " " << MAP_ROWS << "\n";

    for (int y = 0; y < MAP_ROWS; y++) {
        for (int x = 0; x < MAP_COLUMNS; x++) {
            Color pixelColor = GetImageColor(heightmapImage, x, y);
            int height = (pixelColor.r + pixelColor.g + pixelColor.b) / 3;
            file << height;
            if (x < MAP_COLUMNS - 1) file << " ";
        }
        file << "\n";
    }

    std::cout << "Heightmap data saved to: " << filename << std::endl;
}

bool PCG::TerrainManager::LoadHeightmap(const char* filename) {
    Image loadedImage = LoadImage(filename);
    if (loadedImage.data != NULL) {
        CleanupTerrain();
        heightmapImage = loadedImage;
        baseImage = ImageCopy(heightmapImage);
        BuildTerrainFromImage();
        std::cout << "Heightmap loaded from image: " << filename << std::endl;
        return true;
    }

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Failed to load heightmap from: " << filename << std::endl;
        return false;
    }

    int width, height;
    file >> width >> height;

    if (width != MAP_COLUMNS || height != MAP_ROWS) {
        std::cout << "Heightmap dimensions don't match!" << std::endl;
        return false;
    }

    CleanupTerrain();
    heightmapImage = GenImageColor(MAP_COLUMNS, MAP_ROWS, BLACK);

    for (int y = 0; y < MAP_ROWS; y++) {
        for (int x = 0; x < MAP_COLUMNS; x++) {
            int h;
            file >> h;
            h = std::max(0, std::min(255, h));
            Color c = { (unsigned char)h, (unsigned char)h, (unsigned char)h, 255 };
            ImageDrawPixel(&heightmapImage, x, y, c);
        }
    }
    baseImage = ImageCopy(heightmapImage);
    BuildTerrainFromImage();
    std::cout << "Heightmap loaded from data: " << filename << std::endl;
    return true;
}

// =============================================
// GUI Implementation
// =============================================

void PCG::DrawGUI(PCG::TerrainManager& terrainManager) {
    // Generate Terrain Buttons
    if (GuiButton(PCG::RESET_BUTTON_BOUNDS, "New Perlin Terrain")) {
        terrainManager.GenerateFromPerlinNoise();
    }

    Rectangle cellularRect = { PCG::BUTTON_X, PCG::BUTTON_Y - 70, PCG::BUTTON_WIDTH, PCG::BUTTON_HEIGHT };
    if (GuiButton(cellularRect, "New Cellular Terrain")) {
        terrainManager.GenerateFromCellularNoise();
    }

    Rectangle randomRect = { PCG::BUTTON_X, PCG::BUTTON_Y - 140, PCG::BUTTON_WIDTH, PCG::BUTTON_HEIGHT };
    if (GuiButton(randomRect, "New Random Terrain")) {
        terrainManager.GenerateRandomTerrain();
    }

    // Save Buttons
    Rectangle saveImageRect = { PCG::BUTTON_X, PCG::BUTTON_Y - 210, PCG::BUTTON_WIDTH, PCG::BUTTON_HEIGHT };
    if (GuiButton(saveImageRect, "Save Heightmap PNG")) {
        terrainManager.SaveHeightmapImage(PCG::HEIGHTMAP_IMAGE_FILENAME);
    }

    Rectangle saveDataRect = { PCG::BUTTON_X, PCG::BUTTON_Y - 280, PCG::BUTTON_WIDTH, PCG::BUTTON_HEIGHT };
    if (GuiButton(saveDataRect, "Save Heightmap Data")) {
        terrainManager.SaveHeightmapData(PCG::HEIGHTMAP_DATA_FILENAME);
    }

    // Load Buttons
    Rectangle loadImageRect = { PCG::BUTTON_X, PCG::BUTTON_Y - 350, PCG::BUTTON_WIDTH, PCG::BUTTON_HEIGHT };
    if (GuiButton(loadImageRect, "Load Heightmap PNG")) {
        if (terrainManager.LoadHeightmap(PCG::HEIGHTMAP_IMAGE_FILENAME)) {
            std::cout << "Heightmap image loaded!" << std::endl;
        }
    }

    Rectangle loadDataRect = { PCG::BUTTON_X, PCG::BUTTON_Y - 420, PCG::BUTTON_WIDTH, PCG::BUTTON_HEIGHT };
    if (GuiButton(loadDataRect, "Load Heightmap Data")) {
        if (terrainManager.LoadHeightmap(PCG::HEIGHTMAP_DATA_FILENAME)) {
            std::cout << "Heightmap data loaded!" << std::endl;
        }
    }

    //Fill and Cap Sliders
    static float valleyFillLevel = 0.0f;
    static float peakCapLevel = 255.0f;

    Rectangle fillSliderRect = { PCG::BUTTON_X, PCG::BUTTON_Y - 490, PCG::BUTTON_WIDTH, PCG::BUTTON_HEIGHT };
    if (GuiSliderBar(fillSliderRect, "Fill Valleys Lvl", NULL, &valleyFillLevel, 0, 255)) {
        int fillLvl = (int)valleyFillLevel;
        int capLvl = (int)peakCapLevel;
        terrainManager.ApplyFillAndCap(fillLvl, capLvl);
    }

    Rectangle capSliderRect = { PCG::BUTTON_X, PCG::BUTTON_Y - 560, PCG::BUTTON_WIDTH, PCG::BUTTON_HEIGHT };
    if (GuiSliderBar(capSliderRect, "Cap Peaks Lvl", NULL, &peakCapLevel, 0, 255)) {
        int fillLvl = (int)valleyFillLevel;
        int capLvl = (int)peakCapLevel;
        terrainManager.ApplyFillAndCap(fillLvl, capLvl);
    }
}