#include <iostream>
#include <string>
#include <cmath>
#include "raylib.h"
#include "rcamera.h"
#include "raymath.h"

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
    float scale;
    Model model;
    float throttle;
    float angle;
    float tempRoll;

    void draw() override
    {
        DrawModel(model, position, scale, GRAY);
    }

    void move() override
    {
        const float maxThrottle = 0.05;
        const float baseSpeed = 0.01;
        const float maxRoll = 30;

        if(IsKeyDown(KEY_W) && throttle <= maxThrottle) {throttle += 0.005;}
        if(IsKeyDown(KEY_A) && tempRoll > -1*maxRoll ) {tempRoll--;}
        if(IsKeyDown(KEY_S) && throttle > -2*baseSpeed) {throttle -= 0.005;}
        if(IsKeyDown(KEY_D) && tempRoll < maxRoll ) {tempRoll++;}

        else if(tempRoll == -1*maxRoll) {angle++;}
        if(tempRoll == maxRoll) {angle--;}
        
        model.transform = MatrixRotateXYZ((Vector3){0.0f, DEG2RAD * angle, DEG2RAD * tempRoll});

        position.x += (baseSpeed + throttle)*sin(angle * DEG2RAD);
        position.z += (baseSpeed + throttle)*cos(angle * DEG2RAD);

        if(tempRoll < 0 && !IsKeyDown(KEY_A)) {tempRoll++;}
        else if(tempRoll > 0 && !IsKeyDown(KEY_D)) {tempRoll--;}

    }

    public:
    MKapal(Vector3 pos, float initAngle) : Kapal(pos)
    {
        model = LoadModel("../assets/main_ship.obj");

        model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = LoadTexture("../assets/main_ship/tex/ship_lambert3_Diffuse.png");
        model.materials[0].maps[MATERIAL_MAP_ROUGHNESS].texture = LoadTexture("../assets/main_ship/tex/ship_lambert3_Glossiness.png");
        model.materials[0].maps[MATERIAL_MAP_HEIGHT].texture = LoadTexture("../assets/main_ship/tex/ship_lambert3_Height.png");
        model.materials[0].maps[MATERIAL_MAP_NORMAL].texture = LoadTexture("../assets/main_ship/tex/ship_lambert3_Normal.png");
        

        scale = 0.03f;
        angle = initAngle + 90;

        camera.position = (Vector3){ pos.x-10.0f, pos.y+10.0f, pos.z};
        camera.target = position;
        camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
        camera.fovy = 45.0f;                                // Camera field-of-view Y
        camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

        throttle = 0;
        tempRoll = 0;
    }

    Camera camera;

    void update()
    {
        move();
        draw();
        camera.position = { position.x - 10.0f, position.y + 10.0f, position.z };
        camera.target = position;
    }

    ~MKapal() {UnloadModel(model);}
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

    MKapal main_kapal({0, 1.5, 0}, 0);
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