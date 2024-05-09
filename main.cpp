#include <iostream>
#include <string>
#include "raylib.h"
#include "rcamera.h"

const int screenWidth = 2560;
const int screenHeight = 1600;

enum {MENU = 0, SETTING, GAMEPLAY, PAUSE, DEAD};

class Kapal
{
    private:

    protected:
    Vector3 position;
    virtual void draw() {DrawCube(position, 1.0f, 2.0f, 2.0f, RED);}
    virtual void move() {}
    
    public:
    Kapal(Vector3 pos) : position(pos) {}

};

class MKapal : Kapal
{
    private:
    void draw() override
    {
        DrawCube(position, 2.0f, 2.0f, 1.0f, BROWN);
    }

    void move() override
    {
        if(IsKeyDown(KEY_W)) {position.x += 0.3;}
        if(IsKeyDown(KEY_A)) {position.z -= 0.3;}
        if(IsKeyDown(KEY_S)) {position.x -= 0.3;}
        if(IsKeyDown(KEY_D)) {position.z += 0.3;}
    }

    public:
    MKapal(Vector3 pos) : Kapal(pos)
    {
        camera.position = (Vector3){ pos.x-10.0f, pos.y+10.0f, pos.z};
        camera.target = position;
        camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
        camera.fovy = 45.0f;                                // Camera field-of-view Y
        camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type
    }

    Camera camera;

    void update()
    {
        move();
        draw();
        camera.position = { position.x - 10.0f, position.y + 10.0f, position.z };
        camera.target = position;
    }

};

int scrSize(int pixLen, char axis)
{
    const float maxScrX = 2560;
    const float maxScrY = 1600;
    if(axis == 'x') {return (int)((float)pixLen/maxScrX)*(float)screenWidth;}
    else if(axis == 'y') {return (int)((float)pixLen/maxScrY)*(float)screenHeight;}
    std::cout<<"invalid screen axis!!!\n";
    std::exit(1);
}

int main(void)
{
    InitWindow(screenWidth, screenHeight, "KAPAL");

    MKapal main_kapal({0, 0, 0});
    int gamestate = GAMEPLAY;

    ToggleFullscreen();

    SetTargetFPS(60);
    
    while (!WindowShouldClose())
    {

        BeginDrawing();
        
        switch (gamestate)
        {
        case MENU:
            break;
        case SETTING:
            break;
        case GAMEPLAY:
            ClearBackground((Color){50, 80, 255});

            BeginMode3D(main_kapal.camera);

                main_kapal.update();

                DrawGrid(1000, 1);

            EndMode3D();

            break;
        }
        

        EndDrawing();
    }
    CloseWindow();
    return 0;
}