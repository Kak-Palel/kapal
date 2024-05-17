#include <iostream>
#include <string>
#include <cmath>
#include <vector>
#include "raylib.h"
#include "rcamera.h"
#include "raymath.h"

const int screenWidth = 2560;
const int screenHeight = 1600;

enum {MENU = 0, SETTING, GAMEPLAY, PAUSE, DEAD};

class MyCam
{
    private:
    Camera* cam;

    public:
    MyCam(Vector3 target)
    {
        cam  = new Camera();
        cam->position = (Vector3){target.x-10.0f, target.y+10.0f, target.z};
        cam->target = target;
        cam->up = (Vector3){ 0.0f, 1.0f, 0.0f };
        cam->fovy = 45.0f;                                // Camera field-of-view Y
        cam->projection = CAMERA_PERSPECTIVE;             // Camera projection type

    }

    float getdist(Vector3 point)
    {
        return Vector3Distance(point, cam->position);
    }

    void viewScope(std::vector<Vector3>& arr)
    {
        const float xUpperDist = cam->position.y * tan((67.5f)*DEG2RAD);
        const float xLowerDist = cam->position.y * tan((22.5f)*DEG2RAD);
        const float upperDist = cam->position.y / cos((67.5f)*DEG2RAD);
        const float lowerDist = cam->position.y / cos((22.5f)*DEG2RAD);
        const float zHalfUpperDist = upperDist * tan((cam->fovy * ((float)GetScreenWidth()/(float)GetScreenHeight())/2)*DEG2RAD);
        const float zHalfLowerDist = lowerDist * tan((cam->fovy * ((float)GetScreenWidth()/(float)GetScreenHeight())/2)*DEG2RAD);
        
        arr[0] = {cam->position.x + xUpperDist, 0, cam->position.z - zHalfUpperDist};
        arr[1] = {cam->position.x + xUpperDist, 0, cam->position.z + zHalfUpperDist};
        arr[2] = {cam->position.x + xLowerDist, 0, cam->position.z + zHalfLowerDist};
        arr[3] = {cam->position.x + xLowerDist, 0, cam->position.z - zHalfLowerDist};
        
    }

    void setTarget(float x, float y, float z)
    {
        cam->target = (Vector3){x, y, z};
    }

    void setPos(float x, float y, float z)
    {
        cam->position = (Vector3){x, y, z};
    }

    Camera* getCam()
    {
        return cam;
    }

    Vector3 getPos()
    {
        return cam->position;
    }

};

class Ocean {
private:
    int maxWave;
    int waveCount;
    float waveDensity;
    MyCam* cam;
    float waveSpeed;
    std::vector<Vector3> scope;
    std::vector<Vector3*> wavePos;

    void createWave(int wave_count)
    {
        for(int i = 0; i < wave_count; i++) {
            Vector3* temp = new Vector3();
            temp->x = GetRandomValue(scope[0].x, scope[2].x);
            temp->y = 0;
            temp->z = GetRandomValue(scope[0].z, scope[1].z);
            wavePos.push_back(temp);
            waveCount++;
        }
        std::cout<<"waveCount: "<<waveCount<<std::endl;
    }


public:
    Ocean(int max_wave, MyCam* camera , float wave_speed, float wave_density)
    : maxWave(max_wave), waveSpeed(wave_speed), cam(camera), waveDensity(wave_density) {
        waveCount = 0;
        scope = {(Vector3){0, 0, 0}, (Vector3){0, 0, 0}, (Vector3){0, 0, 0}, (Vector3){0, 0, 0}};
        cam->viewScope(scope);
        createWave(waveDensity*(scope[0].x - scope[2].x)*(scope[1].z - scope[0].z));

        //LOAD WAVE MODEL
    }

    void update()
    {
        cam->viewScope(scope);
        for(int i = 0; i < waveCount; i++) {
            wavePos[i]->x += waveSpeed;

            if(wavePos[i]->x > scope[0].x + 2) {
                wavePos[i]->x = scope[2].x - 2;
                wavePos[i]->z = GetRandomValue(scope[0].z, scope[1].z);
            }
            else if (wavePos[i]->x < scope[2].x - 2) {
                wavePos[i]->x = scope[0].x + 2;
                wavePos[i]->z = GetRandomValue(scope[0].z, scope[1].z);
            }
            else if (wavePos[i]->z > scope[1].z + 2) {
                wavePos[i]->z = scope[0].z + 2;
                wavePos[i]->x = GetRandomValue(scope[2].x, scope[0].x);
            }
            else if (wavePos[i]->z < scope[0].z - 2) {
                wavePos[i]->z = scope[1].z - 2;
                wavePos[i]->x = GetRandomValue(scope[2].x, scope[0].x);
            }
        }
    }

    void drawWaves()
    {
        for(int i = 0; i < waveCount; i++) {
            DrawCube(*wavePos[i], 0.5f, 0.5f, 0.5f, WHITE);
        }
    }

    ~Ocean() {
        while(!wavePos.empty())
        {
            delete wavePos.back();
            wavePos.pop_back();
        }
        //UNLOAD WAVE MODEL
    }

};

class Kapal
{
    private:

    protected:
    Vector3 position;
    virtual void draw() {DrawCube(position, 1.0f, 2.0f, 2.0f, RED);}
    virtual void move() {}
    
    public:
    Kapal(Vector3 pos) : position(pos) {}
    virtual ~Kapal() {}
};

class MKapal : Kapal
{
private:
    float scale;
    Model deck;
    Model railing;
    float throttle;
    float dist_cam2kapal;
    MyCam* camera;

    float angle;
    float buoyancyPeriod;
    float buoyancyAngle;
    Vector3 localAxis[3];
    float tempRoll;

    void determineLocalAxis()
    {
        localAxis[0] = Vector3Normalize((Vector3){position.x*sin((angle)*DEG2RAD), 0, position.z*cos((angle)*DEG2RAD)});
        localAxis[1] = (Vector3){0, 1, 0};
        localAxis[2] = Vector3Normalize(Vector3CrossProduct(localAxis[0], localAxis[1]));
    }
public:
    MKapal(Vector3 pos, float initAngle, MyCam* cam) : Kapal(pos)
    {
        camera = cam;
        
        railing = LoadModel("assets/obj/ship/railing.obj");
        std::cout<<"railing: "<<railing.meshCount<<std::endl;
        deck = LoadModel("assets/obj/ship/deck.obj");
        std::cout<<"deck: "<<deck.meshCount<<std::endl;

        deck.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = LoadTexture("assets/tex/ship/deck.png");
        railing.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = LoadTexture("assets/tex/ship/railing.png");

        scale = 0.25f;
        angle = initAngle + 90;

        throttle = 0;
        tempRoll = 0;

        buoyancyPeriod = 0;
        buoyancyAngle = 0;

        localAxis[0] = {1, 0, 0};
        localAxis[1] = {0, 1, 0};   
        localAxis[2] = {0, 0, 1};
    }

    void draw() override
    {
        DrawModel(deck, {position.x, position.y + 1.5f, position.z}, scale, GRAY);
        DrawModel(railing, {position.x, position.y + 1.5f, position.z}, scale, WHITE);
    }

    void move() override
    {
        const float maxThrottle = 0.075;
        const float baseSpeed = 0.005;
        const float maxRoll = 15;
        
        //movement angle calculation
        if(IsKeyDown(KEY_W) && throttle <= maxThrottle) {throttle += 0.001;}
        if(IsKeyDown(KEY_A) && tempRoll > -1*maxRoll ) {tempRoll -= (baseSpeed + throttle) * 3;}
        if(IsKeyDown(KEY_S) && throttle > -2*baseSpeed) {throttle -= 0.001;}
        if(IsKeyDown(KEY_D) && tempRoll < maxRoll ) {tempRoll += (baseSpeed + throttle) * 3;}
        if(tempRoll > maxRoll) {tempRoll = maxRoll;}
        else if(tempRoll < -1*maxRoll) {tempRoll = -1*maxRoll;}
        if(tempRoll >= maxRoll/2) {angle -= (baseSpeed + throttle) * 8 * abs(sin(6*tempRoll));}
        else if(tempRoll <= -1*maxRoll/2) {angle += (baseSpeed + throttle) * 8 * abs(sin(6*tempRoll));}

        GetFrameTime();

        //buoyancy angle calculation
        buoyancyPeriod += 0.5*(baseSpeed + throttle);
        position.y = 0.5 + 0.5*sin(0.1*buoyancyPeriod);
        buoyancyAngle = 10*sin(buoyancyPeriod);
        // if(buoyancyPeriod >= 2*PI) {buoyancyPeriod = 0;}
        
        //model rotation using local axis
        deck.transform = MatrixIdentity();
        deck.transform = MatrixMultiply(deck.transform, MatrixRotate(localAxis[1], DEG2RAD * angle));
        deck.transform = MatrixMultiply(deck.transform, MatrixRotate(localAxis[0], DEG2RAD * -1* tempRoll));
        deck.transform = MatrixMultiply(deck.transform, MatrixRotate(localAxis[2], DEG2RAD * buoyancyAngle));
        railing.transform = MatrixIdentity();
        railing.transform = MatrixMultiply(railing.transform, MatrixRotate(localAxis[1], DEG2RAD * angle));
        railing.transform = MatrixMultiply(railing.transform, MatrixRotate(localAxis[0], DEG2RAD * -1* tempRoll));
        railing.transform = MatrixMultiply(railing.transform, MatrixRotate(localAxis[2], DEG2RAD * buoyancyAngle));

        position.x += (baseSpeed + throttle) * sin(angle * DEG2RAD);
        position.z += (baseSpeed + throttle) * cos(angle * DEG2RAD);
        CameraMoveForward(camera->getCam(), (baseSpeed + throttle)*sin(angle * DEG2RAD), true);
        CameraMoveRight(camera->getCam(), (baseSpeed + throttle)*cos(angle * DEG2RAD) , true);

        camera->setTarget(position.x, 0, position.z);

        if(tempRoll < 0 && !IsKeyDown(KEY_A)) {tempRoll += (baseSpeed + throttle) * 3;}
        else if(tempRoll > 0 && !IsKeyDown(KEY_D)) {tempRoll -= (baseSpeed + throttle) * 3;}

        dist_cam2kapal = Vector3Distance(position, camera->getPos());
        float zoomMove = GetMouseWheelMove();
        
        if((dist_cam2kapal > 2 && zoomMove > 0) || (dist_cam2kapal < 30 && zoomMove < 0))
        {
            CameraMoveForward(camera->getCam(), zoomMove, false);
        }
        determineLocalAxis();
    }

    Vector3 getPos()
    {
        return position;
    }

    ~MKapal()
    {
        UnloadTexture(deck.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture);
        UnloadTexture(railing.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture);
        UnloadModel(deck);
        UnloadModel(railing);
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

    MyCam camera({0, 0, 0});
    MKapal main_kapal({0, 1.5, 0}, 0, &camera);
    Ocean ocean(100, &camera, 0.01, 0.1);
    int gamestate = GAMEPLAY;

    float deltaTime;
    ToggleFullscreen();
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        deltaTime = GetFrameTime();

        BeginDrawing();
        
        switch (gamestate)
        {
        case MENU:
            break;
        case SETTING:
            break;
        case GAMEPLAY:
            ClearBackground((Color){29,162,216});

            BeginMode3D(*(camera.getCam()));
                //game update
                main_kapal.move();
                ocean.update();

                //game draw
                DrawGrid(1000, 1);
                main_kapal.draw();
                ocean.drawWaves();

            EndMode3D();

            break;
        }

        EndDrawing();

    }
    CloseWindow();
    return 0;
}