#include <iostream>
#include <string>
#include <cmath>
#include <vector>
#include "raylib.h"
#include "rcamera.h"
#include "raymath.h"

const int screenWidth = 2560;
const int screenHeight = 1600;
const float GRAVITY = 0.098f;

const Color SEABLUE = {29,162,216};
// const float GRAVITY = 0.00f;

enum {MENU = 0, SETTING, GAMEPLAY, PAUSE, DEAD};
unsigned long long int frameCounter = 0;

struct ShipHitbox
{
    BoundingBox rail;
    BoundingBox deck;
    float* health;
};
std::vector<ShipHitbox*> ShipHitboxes;

int scrSize(int pixLen, char axis);
Vector3 normalizeVector3(Vector3 v);

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
    Model waveModel;
    std::vector<Vector3> scope;
    std::vector<Vector3> tempScope;
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
    }

    void createWave(std::vector<Vector3>& oldScope)
    {
        int targetWave = waveDensity*(scope[0].x - scope[2].x)*(scope[1].z - scope[0].z);
        if((oldScope[0].x - oldScope[2].x)*(oldScope[1].z - oldScope[0].z) - (scope[0].x - scope[2].x)*(scope[1].z - scope[0].z) > 1)
        {
            while(waveCount > targetWave)
            {
                delete wavePos[wavePos.size() - 1];
                wavePos.erase(wavePos.end());
                waveCount--;
            }
            return;
        }

        for(int i = 0; (i <= targetWave - waveCount); i ++)
        {
            switch(i%4)
            {
                case 0:
                    createWave("bottom");
                    break;
                case 1:
                    createWave("top");
                    break;
                case 2:
                    createWave("left");
                    break;
                case 3:
                    createWave("right");
                    break;
            }
        }
    }

    void createWave(const std::string side)
    {
        if(side == "bottom")
        {
            // std::cout<<"bottom\n";
            Vector3* temp = new Vector3();
            temp->x = scope[2].x - GetRandomValue(1, 4);
            temp->y = 0;
            temp->z = GetRandomValue(scope[0].z, scope[1].z);
            wavePos.push_back(temp);
            waveCount++;
        }
        else if(side == "top")
        {
            // std::cout<<"top\n";
            Vector3* temp = new Vector3();
            temp->x = scope[0].x + GetRandomValue(1, 4);
            temp->y = 0;
            temp->z = GetRandomValue(scope[0].z, scope[1].z);
            wavePos.push_back(temp);
            waveCount++;
        }
        else if(side == "left")
        {
            // std::cout<<"left\n";
            Vector3* temp = new Vector3();
            temp->x = GetRandomValue(scope[2].x, scope[0].x);
            temp->y = 0;
            temp->z = scope[0].z + GetRandomValue(1, 4);
            wavePos.push_back(temp);
            waveCount++;
        }
        else if(side == "right")
        {
            // std::cout<<"right\n";
            Vector3* temp = new Vector3();
            temp->x = GetRandomValue(scope[2].x, scope[0].x);
            temp->y = 0;
            temp->z = scope[1].z - GetRandomValue(1, 4);
            wavePos.push_back(temp);
            waveCount++;
        }
    }

public:
    Ocean(int max_wave, MyCam* camera , float wave_speed, float wave_density)
    : maxWave(max_wave), waveSpeed(wave_speed), cam(camera), waveDensity(wave_density) {
        waveCount = 0;
        scope = {(Vector3){0, 0, 0}, (Vector3){0, 0, 0}, (Vector3){0, 0, 0}, (Vector3){0, 0, 0}};
        cam->viewScope(scope);
        tempScope = scope;
        createWave(waveDensity*(scope[0].x - scope[2].x)*(scope[1].z - scope[0].z));

        waveModel = LoadModel("assets/obj/wave.obj");
        waveModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = LoadTexture("assets/tex/wave.png");
    }

    void update()
    {
        cam->viewScope(scope);
        
        for(int i = 0; i < wavePos.size(); i++)
        {
            wavePos[i]->x += waveSpeed;

            if(wavePos[i]->x > scope[0].x + 5) {
                // std::cout<<"deleting top\n";
                delete wavePos[i];
                wavePos.erase(wavePos.begin() + i);
                if(tempScope[0].x - scope[0].x > 1){createWave("bottom");}
            }
            else if (wavePos[i]->x < scope[2].x - 5) {
                // std::cout<<"deleting bottom\n";
                delete wavePos[i];
                wavePos.erase(wavePos.begin() + i);
                if(tempScope[0].x - scope[0].x > 1){createWave("top");}
            }
            
            if(wavePos[i]->z > scope[1].z + 5) {
                // std::cout<<"deleting right\n";
                delete wavePos[i];
                wavePos.erase(wavePos.begin() + i);
                createWave("left");
            }
            else if (wavePos[i]->z < scope[0].z - 5) {
                // std::cout<<"deleting left\n";
                delete wavePos[i];
                wavePos.erase(wavePos.begin() + i);
                createWave("right");
            }

        }
        waveCount = wavePos.size();
        createWave(tempScope);
        tempScope = scope;
    }

    void drawWaves()
    {
        for(int i = 0; i < waveCount; i++) {
            DrawModel(waveModel, *wavePos[i], 1.0f, WHITE);
        }
    }

    ~Ocean() {
        while(!wavePos.empty())
        {
            delete wavePos.back();
            wavePos.pop_back();
        }

        UnloadTexture(waveModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture);
        UnloadModel(waveModel);
    }
};

class Bullet
{
    private:
    Vector3 position;
    Vector3 direction;
    float speed;
    float damage;
    float downSpeed;
    bool isAlive;
    float radius;
    std::vector<Vector3> trails;

    public:
    Bullet(Vector3 pos, Vector3 directionVec, float speed = 0.3, float dmg = 10, float radius = 0.25) : position(pos), damage(dmg), isAlive(true)
    {
        this->speed = speed;
        this->direction = normalizeVector3(directionVec);

        this->downSpeed = -1*directionVec.y;

        this->radius = radius;
    }
    ~Bullet() {}

    void update()
    {
        position.x += direction.x * speed;
        position.z += direction.z * speed;

        position.y -= downSpeed;
        downSpeed += GRAVITY/60;

        if(position.y < -1) {isAlive = false; return;}

        //check collision
        for(int i = 0; i < ShipHitboxes.size(); i++)
        {
            if(CheckCollisionBoxSphere(ShipHitboxes[i]->rail, position, radius) || CheckCollisionBoxSphere(ShipHitboxes[i]->deck, position, radius))
            {
                *(ShipHitboxes[i]->health) -= damage;
                isAlive = false;
            }
        }

    }

    void draw()
    {
        DrawSphere(position, radius, BLACK);
        if(frameCounter%7 == 0)
        {
            trails.push_back(position);
        }

        for(int i = 0; i < trails.size(); i++)
        {
            DrawSphere(trails[i], radius - 0.025*i, GRAY);
        }
    }

    bool alive()
    {
        return isAlive;
    }

    void kill()
    {
        isAlive = false;
    }

    Vector3 getPos()
    {
        return position;
    }

};
std::vector<Bullet*> Bullets;


class Kapal
{
    private:

    protected:
    Model model;

    float scale;
    ShipHitbox hitboxes;
    
    float health;

    Vector3 position;
    Vector3 localAxis[3];
    float angle;
    float buoyancyPeriod;
    float buoyancyAngle;
    float tempRoll;
    float throttle;
    
    int cooldown;
    int cooldownTimer_R;
    int cooldownTimer_L;

    // virtual void draw() {DrawCube(position, 1.0f, 2.0f, 2.0f, RED);}
    virtual void move() {}
    
    void determineLocalAxis()
    {
        localAxis[0] = normalizeVector3((Vector3){position.x*sin((angle)*DEG2RAD), 0, position.z*cos((angle)*DEG2RAD)});
        localAxis[1] = (Vector3){0, 1, 0};
        localAxis[2] = normalizeVector3(Vector3CrossProduct(localAxis[0], localAxis[1]));

        if(angle < 0)
        {
            angle = 360 + angle;
        }
        if(angle >= 360)
        {
            angle -= 360;
        }
    }

    public:
    Kapal(Vector3 pos) : position(pos)
    {
        cooldown = 60;
        cooldownTimer_R = 0;
        cooldownTimer_L = 0;
        model = LoadModel("assets/obj/ship/allShip.obj");
    }

    void shoot(bool right)
    {
        if(right && cooldownTimer_R > 0) {return;}
        else if(!right && cooldownTimer_L > 0) {return;}
        
        if(right) {cooldownTimer_R = cooldown;}
        else {cooldownTimer_L = cooldown;}
        
        Vector3 bulletPos = position;
        bulletPos.y += 0.5;
        bulletPos.z -= 1.5*cos((angle)*DEG2RAD);
        bulletPos.x -= 1.5*sin((angle)*DEG2RAD);
        
        Vector3 bulletDir;
        bulletDir.x = sin((angle - 90)*DEG2RAD);
        bulletDir.z = cos((angle - 90)*DEG2RAD);
        bulletDir.y = sin(tempRoll*DEG2RAD);

        if(!right)
        {
            bulletDir.x *= -1;
            bulletDir.y *= -1;
            bulletDir.z *= -1;
        }
        
        Bullet* temp = new Bullet(bulletPos, bulletDir);
        Bullets.push_back(temp);
    }
    
    void draw()
    {
        DrawModel(model, {position.x, position.y + 1.5f, position.z}, scale, WHITE);
    }
    
    Vector3 getPos()
    {
        return position;
    }

    float getAngle()
    {
        return angle;
    }

    ~Kapal()
    {
        UnloadModel(model);
    }
};

class MKapal : public Kapal
{
private:

    float dist_cam2kapal;
    MyCam* camera;


public:
    MKapal(Vector3 pos, float initAngle, MyCam* cam) : Kapal(pos)
    {
        camera = cam;

        hitboxes.health = &health;
        ShipHitboxes.push_back(&hitboxes);

        scale = 0.25f;
        angle = 90 + initAngle;

        throttle = 0;
        tempRoll = 0;

        buoyancyPeriod = 0;
        buoyancyAngle = 0;

        localAxis[0] = {1, 0, 0};
        localAxis[1] = {0, 1, 0};   
        localAxis[2] = {0, 0, 1};
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
        buoyancyPeriod += 0.025;
        position.y = 0.5 + 0.5*sin(0.1*buoyancyPeriod);
        buoyancyAngle = 10*sin(buoyancyPeriod);
        // if(buoyancyPeriod % (2*PI) && buoyancyPeriod != 0) {buoyancyPeriod = 0;}

        model.transform = MatrixIdentity();
        model.transform = MatrixMultiply(model.transform, MatrixRotate(localAxis[1], DEG2RAD * angle));
        model.transform = MatrixMultiply(model.transform, MatrixRotate(localAxis[0], DEG2RAD * -1* tempRoll));
        model.transform = MatrixMultiply(model.transform, MatrixRotate(localAxis[2], DEG2RAD * buoyancyAngle));

        position.x += (baseSpeed + throttle) * sin(angle * DEG2RAD);
        position.z += (baseSpeed + throttle) * cos(angle * DEG2RAD);
        CameraMoveForward(camera->getCam(), (baseSpeed + throttle)*sin(angle * DEG2RAD), true);
        CameraMoveRight(camera->getCam(), (baseSpeed + throttle)*cos(angle * DEG2RAD) , true);

        camera->setTarget(position.x, 0, position.z);

        if(tempRoll < 0 && !IsKeyDown(KEY_A)) {tempRoll += (baseSpeed + throttle) * 3;}
        else if(tempRoll > 0 && !IsKeyDown(KEY_D)) {tempRoll -= (baseSpeed + throttle) * 3;}

        dist_cam2kapal = Vector3Distance(position, camera->getPos());
        float zoomMove = GetMouseWheelMove();
        
        if((dist_cam2kapal > 2 && zoomMove > 0) || (dist_cam2kapal < 100 && zoomMove < 0))
        {
            CameraMoveForward(camera->getCam(), zoomMove, false);
        }
        determineLocalAxis();

        //shoot
        if(IsKeyReleased(KEY_RIGHT)){shoot(true);}
        if(IsKeyReleased(KEY_LEFT)){shoot(false);}

        if(cooldownTimer_R > 0) {cooldownTimer_R--;}
        if(cooldownTimer_L > 0) {cooldownTimer_L--;}
    }
};

class EKapal : public Kapal
{
    private:
    Kapal* target;
    float angleToFace;

    std::vector<bool> control()
    {
        //TODO: FIX AI-------------------------------------------------------------------------------------------------------------------
        const float minDistToAttack = 0.0f;
        const float angleTolerance = 15;
        std::vector<bool> movement = {false,  //W 
                                      false,  //A
                                      false,  //S
                                      false}; //D

        angleToFace = atan2f(position.x - target->getPos().x, position.z - target->getPos().z)*RAD2DEG + 180;

        if(Vector3Distance(target->getPos(), this->position) >= minDistToAttack)
        {
            movement[0] = true;
            float angleBetween = this->angleToFace - this->angle;
            if(angleBetween <= 180 && abs(angleBetween) > 5 && angleBetween > 0)
            {
                movement[1] = true;
            }
            else if((angleBetween > 180 || angleBetween < 0) && abs(angleBetween) > 5)
            {
                movement[3] = !movement[1];
            }
        }
        return movement;
    }

    public:
    void move() override
    {
        const float maxThrottle = 0.075;
        const float baseSpeed = 0.005;
        const float maxRoll = 15;

        std::vector<bool> movement = control();

        //movement angle calculation
        if(movement[0] && throttle <= maxThrottle) {throttle += 0.001;}
        if(movement[1] && tempRoll > -1*maxRoll ) {tempRoll -= (baseSpeed + throttle) * 3;}
        if(movement[2] && throttle > -2*baseSpeed) {throttle -= 0.001;}
        if(movement[3] && tempRoll < maxRoll ) {tempRoll += (baseSpeed + throttle) * 3;}
        
        if(tempRoll > maxRoll) {tempRoll = maxRoll;}
        else if(tempRoll < -1*maxRoll) {tempRoll = -1*maxRoll;}
        if(tempRoll >= maxRoll/2) {angle -= (baseSpeed + throttle) * 8 * abs(sin(6*tempRoll));}
        else if(tempRoll <= -1*maxRoll/2) {angle += (baseSpeed + throttle) * 8 * abs(sin(6*tempRoll));}

        GetFrameTime();

        //buoyancy angle calculation
        buoyancyPeriod += 0.025;
        position.y = 0.5 + 0.5*sin(0.1*buoyancyPeriod);
        buoyancyAngle = 10*sin(buoyancyPeriod);
        // if(buoyancyPeriod % (2*PI) && buoyancyPeriod != 0) {buoyancyPeriod = 0;}

        model.transform = MatrixIdentity();
        model.transform = MatrixMultiply(model.transform, MatrixRotate(localAxis[1], DEG2RAD * angle));
        model.transform = MatrixMultiply(model.transform, MatrixRotate(localAxis[0], DEG2RAD * -1* tempRoll));
        model.transform = MatrixMultiply(model.transform, MatrixRotate(localAxis[2], DEG2RAD * buoyancyAngle));

        position.x += (baseSpeed + throttle) * sin(angle * DEG2RAD);
        position.z += (baseSpeed + throttle) * cos(angle * DEG2RAD);

        if(tempRoll < 0 && !movement[1]) {tempRoll += (baseSpeed + throttle) * 3;}
        else if(tempRoll > 0 && !movement[3]) {tempRoll -= (baseSpeed + throttle) * 3;}

        determineLocalAxis();

        //shoot
        if(IsKeyReleased(KEY_RIGHT)){shoot(true);}
        if(IsKeyReleased(KEY_LEFT)){shoot(false);}

        if(cooldownTimer_R > 0) {cooldownTimer_R--;}
        if(cooldownTimer_L > 0) {cooldownTimer_L--;}
    }

    float targetAng(){return angleToFace;}

    EKapal(Vector3 pos, float initAngle, Kapal* target) : Kapal(pos)
    {
        this->target = target;
        scale = 0.25f;
        angle = 90 + initAngle;

        hitboxes.health = &health;
        ShipHitboxes.push_back(&hitboxes);

        throttle = 0;
        tempRoll = 0;

        buoyancyPeriod = 0;
        buoyancyAngle = 0;

        localAxis[0] = {1, 0, 0};
        localAxis[1] = {0, 1, 0};   
        localAxis[2] = {0, 0, 1};
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

Vector3 normalizeVector3(Vector3 v)
{
    float length = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    return {v.x/length, v.y/length, v.z/length};
}

int main(void)
{
    InitWindow(screenWidth, screenHeight, "KAPAL");

    MyCam camera({0, 0, 0});
    std::cout<<"starting loop1\n";
    MKapal main_kapal({0, 1.5, 0}, 0, &camera);
    EKapal enemy_kapal({10, 1.5, 10}, 0, &main_kapal);
    std::cout<<"starting loop2\n";
    Ocean ocean(100, &camera, 0.01, 0.025);
    std::cout<<"starting loop3\n";
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
            ClearBackground(SEABLUE);

            BeginMode3D(*(camera.getCam()));
                //game update
                main_kapal.move();
                enemy_kapal.move();
                ocean.update();
                for(int i = 0; i < Bullets.size(); i++)
                {
                    Bullets[i]->update();
                    if(!Bullets[i]->alive())
                    {
                        delete Bullets[i];
                        Bullets.erase(Bullets.begin() + i);
                    }
                }

                //game draw
                // DrawGrid(1000, 1);
                for(int i = 0; i < Bullets.size(); i++)
                {
                    Bullets[i]->draw();
                }
                main_kapal.draw();
                enemy_kapal.draw();
                ocean.drawWaves();

            EndMode3D();

            DrawText(TextFormat("angle2see: %f", enemy_kapal.targetAng()), 10, 10, 50, RED);
            DrawText(TextFormat("anglecurr: %f", enemy_kapal.getAngle()), 10, 60, 50, RED);

            break;
        }

        EndDrawing();

        frameCounter++;
    }
    CloseWindow();
    return 0;
}