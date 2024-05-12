#include <iostream>
#include <string>
#include <cmath>
#include <complex>
#include <vector>
#include "raylib.h"
#include "rcamera.h"
#include "raymath.h"
#include "rlgl.h"

const int screenWidth = 2560;
const int screenHeight = 1600;

enum {MENU = 0, SETTING, GAMEPLAY, PAUSE, DEAD};

class Ocean {
private:
    int size;
    float *heightMap;
    Shader shader;
    Mesh mesh;
    Texture2D texture;
    RenderTexture2D renderTexture;

public:
    Ocean(int size) : size(size) {
        heightMap = new float[size * size];
        GenerateHeightMap();

        shader = LoadShader("assets/ocean.vs", "assets/ocean.fs");
        shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(shader, "matModel");
        shader.locs[SHADER_LOC_MATRIX_VIEW] = GetShaderLocation(shader, "matView");
        shader.locs[SHADER_LOC_MATRIX_PROJECTION] = GetShaderLocation(shader, "matProjection");
        shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");

        GenerateMesh();
        GenerateTexture();
    }

    ~Ocean() {
        delete[] heightMap;
        UnloadTexture(texture);
        UnloadRenderTexture(renderTexture);
        UnloadShader(shader);   
    }

    void GenerateHeightMap() {
        for (int x = 0; x < size; x++) {
            for (int z = 0; z < size; z++) {
                heightMap[x * size + z] = sin(x * 0.1f) * cos(z * 0.1f) * 5.0f;
            }
        }
    }

    void GenerateMesh() {
        mesh = GenMeshPlane(size, size, size, size);
    }

    void GenerateTexture() {
        renderTexture = LoadRenderTexture(size, size);
        BeginTextureMode(renderTexture);
        ClearBackground(BLANK);
        EndTextureMode();

        // Create a texture from the render texture's texture
        Image image = LoadImageFromTexture(renderTexture.texture);
        texture = LoadTextureFromImage(image);
        UnloadImage(image);
    }


    void Update() {
        // Update ocean simulation here
    }

    void Draw(Camera* camera) {
        // Draw ocean surface
        BeginShaderMode(shader);
        SetShaderValueMatrix(shader, shader.locs[SHADER_LOC_MATRIX_MODEL], MatrixIdentity());
        SetShaderValueMatrix(shader, shader.locs[SHADER_LOC_MATRIX_VIEW], GetCameraMatrix(*camera));
        SetShaderValueMatrix(shader, shader.locs[SHADER_LOC_MATRIX_PROJECTION], GetCameraMatrix(*camera));
        SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], &(camera->position), SHADER_UNIFORM_VEC3);

        Material material = { 0 };
        material.maps = (MaterialMap*)malloc(11*sizeof(MaterialMap));
        material.shader = shader;
        material.maps[MATERIAL_MAP_DIFFUSE].texture = texture;

        DrawMesh(mesh, material, {0, 0, 0});

        EndShaderMode();
    }


};

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

    void viewScope(Vector3* arr)
    {
        //TODO: get the 4 end point of the field of views
        arr[0] = {0, 0, 0};
        arr[1] = {0, 0, 0};
        arr[2] = {0, 0, 0};
        arr[3] = {0, 0, 0};
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
    float dist_cam2kapal;
    MyCam* camera;

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

        position.x += (baseSpeed + throttle) * sin(angle * DEG2RAD);
        position.z += (baseSpeed + throttle) * cos(angle * DEG2RAD);
        CameraMoveForward(camera->getCam(), (baseSpeed + throttle)*sin(angle * DEG2RAD), true);
        CameraMoveRight(camera->getCam(), (baseSpeed + throttle)*cos(angle * DEG2RAD) , true);

        if(tempRoll < 0 && !IsKeyDown(KEY_A)) {tempRoll++;}
        else if(tempRoll > 0 && !IsKeyDown(KEY_D)) {tempRoll--;}
    }

    public:
    MKapal(Vector3 pos, float initAngle, MyCam* cam) : Kapal(pos)
    {
        camera = cam;
        model = LoadModel("assets/main_ship.obj");

        model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = LoadTexture("assets/main_ship/tex/ship_lambert3_Diffuse.png");
        model.materials[0].maps[MATERIAL_MAP_ROUGHNESS].texture = LoadTexture("assets/main_ship/tex/ship_lambert3_Glossiness.png");
        model.materials[0].maps[MATERIAL_MAP_HEIGHT].texture = LoadTexture("assets/main_ship/tex/ship_lambert3_Height.png");
        model.materials[0].maps[MATERIAL_MAP_NORMAL].texture = LoadTexture("assets/main_ship/tex/ship_lambert3_Normal.png");

        scale = 0.03f;
        angle = initAngle + 90;

        throttle = 0;
        tempRoll = 0;
    }


    void update()
    {
        move();
        draw();
        dist_cam2kapal = Vector3Distance(position, camera->getPos());
        float zoomMove = GetMouseWheelMove();
        
        if((dist_cam2kapal > 2 && zoomMove > 0) || (dist_cam2kapal < 30 && zoomMove < 0))
        {
            CameraMoveForward(camera->getCam(), zoomMove, false);
        }
    }

    ~MKapal()
    {
        UnloadTexture(model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture);
        UnloadTexture(model.materials[0].maps[MATERIAL_MAP_ROUGHNESS].texture);
        UnloadTexture(model.materials[0].maps[MATERIAL_MAP_HEIGHT].texture);
        UnloadTexture(model.materials[0].maps[MATERIAL_MAP_NORMAL].texture);
        UnloadModel(model);
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
    Ocean ocean(640);
    MKapal main_kapal({0, 1.5, 0}, 0, &camera);
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
            ClearBackground((Color){WHITE});

            BeginMode3D(*(camera.getCam()));

                DrawGrid(1000, 1);
                main_kapal.update();
                ocean.Update();
                ocean.Draw(camera.getCam());
                

            EndMode3D();
            break;
        }

        EndDrawing();

    }
    CloseWindow();
    return 0;
}