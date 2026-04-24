#include "raylib.h"
#include "resource_dir.h"
#include "PCG.h" // Import our new module

int main() {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(PCG::SCREEN_WIDTH, PCG::SCREEN_HEIGHT, "Construct Map Editor");

    PCG::TileMap tileMap;
    //comment out generators you dont want to use 
    //tileMap.SetMapGenerator(new PCG::RandomMapGenerator());
    tileMap.SetMapGenerator(new PCG::NoiseMapGenerator());
    //tileMap.SetMapGenerator(new PCG::GameOfLifeGenerator());
    tileMap.GetMapGenerator()->Generate(tileMap.GetTileData()); // Generate the map using the selected generator

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        tileMap.DrawMap();
        DrawText("Construct Map Editor", 20, 20, 20, WHITE);
        tileMap.DrawGUI();
        EndDrawing();
    }
    CloseWindow();
    return 0;
}