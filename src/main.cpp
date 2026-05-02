#include "raylib.h"
#include "resource_dir.h"
#include "PCG.h" // Import our new module
#include <cstdlib>
#include <iostream>

int main() {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(PCG::SCREEN_WIDTH, PCG::SCREEN_HEIGHT, "Construct Map Editor");

    Camera camera = { 0 };
    camera.position = { 18.0f, 21.0f, 18.0f };
    camera.target = { 0.0f, 0.0f, 0.0f };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    int offsetX = GetRandomValue(0, 1000);
    int offsetY = GetRandomValue(0, 1000);
    float scale = 5.0f;

    Image noiseImage = GenImagePerlinNoise(PCG::MAP_COLUMNS, PCG::MAP_ROWS, offsetX, offsetY, scale);
   // Image noiseImage = GenImageCellular(PCG::MAP_COLUMNS, PCG::MAP_ROWS, 16/*PCG::TILE_SIZE*/);
    //ImageColorContrast(&noiseImage, -25);
    //ImageColorInvert(&noiseImage);


    Color replacement = GRAY;
    unsigned char valleyFillHeight = 130;



    //for (int y = 0; y < noiseImage.width; y++)
    //{
    //    for (int x = 0; x < noiseImage.height; x++)
    //    {
    //        Color col = GetImageColor(noiseImage, x, y);
    //        float brightness = (col.r + col.g + col.b) / (3.0f * 255.0f) * 100;
    //        int intBrightness = (int)brightness;
    //        std::cout << brightness << std::endl;
    //        if (brightness < 55.0f /*&& brightness > 35.0f*/) {
    //            if (brightness != 50.9804)
    //            {
    //                ImageColorReplace(&noiseImage, col, replacement);
    //                std::cout << "is in range" << std::endl;
    //            }
    //        }
    //        else {
    //            std::cout << "not in range" << std::endl;
    //        }
    //    }
    //}

    //fill valleys
    for (int i = 0; i < valleyFillHeight; i++)
    {
        Color col = { i, i, i, 255 };
        ImageColorReplace(&noiseImage, col, {valleyFillHeight, valleyFillHeight, valleyFillHeight, 255});
    }

    ////flatten peaks
    //for (int i = 255; i > 125; i--)
    //{
    //    Color col = { i, i, i, 255 };
    //    ImageColorReplace(&noiseImage, col, replacement);
    //}


    Texture2D noiseTexture = LoadTextureFromImage(noiseImage);

    Mesh terrainMesh = GenMeshHeightmap(noiseImage, PCG::MAP_SIZE);
    Model terrainModel = LoadModelFromMesh(terrainMesh);

    terrainModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = noiseTexture;
    Vector3 mapPos = { (PCG::MAP_SIZE.x * 0.5f) * -1, 0, (PCG::MAP_SIZE.z * 0.5f) * -1 };

    SetTargetFPS(60);


    PCG::TileMap tileMap;
    //comment out generators you dont want to use 
    //tileMap.SetMapGenerator(new PCG::RandomMapGenerator());
    //tileMap.SetMapGenerator(new PCG::NoiseMapGenerator());
    //tileMap.SetMapGenerator(new PCG::GameOfLifeGenerator());
    //tileMap.SetMapGenerator(new PCG::TerrainGenerator());
    //tileMap.GetMapGenerator()->Generate(tileMap.GetTileData()); // Generate the map using the selected generator

    while (!WindowShouldClose()) {

        UpdateCamera(&camera, CAMERA_ORBITAL);

        BeginDrawing();
            ClearBackground(BLACK);
            //tileMap.DrawMap();

            BeginMode3D(camera);
            DrawModel(terrainModel, mapPos, 1.0f, RED);
            DrawGrid(20, 1.0f);
            EndMode3D();

            DrawText("Map Generator", 20, 20, 20, WHITE);
            tileMap.DrawGUI();
            DrawTexture(noiseTexture, PCG::SCREEN_WIDTH - noiseTexture.width - 20, 20, WHITE);
            DrawRectangleLines(PCG::SCREEN_WIDTH - noiseTexture.width - 20, 20, noiseTexture.width, noiseTexture.height, GREEN);
            
        EndDrawing();
    }

    UnloadTexture(noiseTexture);
    UnloadModel(terrainModel);

    CloseWindow();
    return 0;
}