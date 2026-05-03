#include "raylib.h"
#include "PCG.h"

using namespace PCG;

int main() {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Terrain Heightmap Generator");

    // Setup camera
    Camera camera = { 0 };
    camera.position = { 18.0f, 21.0f, 18.0f };
    camera.target = { 0.0f, 0.0f, 0.0f };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    SetTargetFPS(60);

    // Create terrain manager and generate initial terrain
    TerrainManager terrainManager;
    terrainManager.GenerateFromPerlinNoise();

    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_ORBITAL);

        BeginDrawing();
        ClearBackground(BLACK);

        // Draw 3D terrain
        BeginMode3D(camera);
        terrainManager.Draw(camera);
        EndMode3D();

        // Draw UI
        DrawText("Terrain Heightmap Generator", 20, 20, 20, WHITE);
        DrawGUI(terrainManager);

        // Draw minimap preview
        Texture2D heightmapTex = terrainManager.GetTexture();
        if (heightmapTex.id > 0) {
            int previewX = SCREEN_WIDTH - heightmapTex.width - 20;
            int previewY = 20;
            DrawTexture(heightmapTex, previewX, previewY, WHITE);
            DrawRectangleLines(previewX, previewY, heightmapTex.width, heightmapTex.height, GREEN);
            DrawText("Heightmap Preview", previewX, previewY - 20, 10, WHITE);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}