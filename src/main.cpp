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

enum {MENU = 0, SETTING, GAMEPLAY, PAUSE, DEAD};
unsigned long long int frameCounter = 0;

class Kapal;
class MKapal;
class EKapal;
class Ocean;
class Bullet;

struct ShipHitbox
{
    BoundingBox ship;
    float* health;
    Kapal* owner;
};
std::vector<ShipHitbox*> ShipHitboxes;

int scrSize(int pixLen, char axis);
Vector3 normalizeVector3(Vector3 v);

class MyCam
{
    private:
    Camera* cam;
    float shakeDuration;
    float shakeIntensity;
    Vector3 shakePos;
    bool shaking;

    public:
    MyCam(Vector3 target)
    {
        cam = new Camera();
        cam->position = (Vector3){target.x-10.0f, target.y+10.0f, target.z};
        cam->target = target;
        cam->up = (Vector3){ 0.0f, 1.0f, 0.0f };
        cam->fovy = 45.0f;                                // Camera field-of-view Y
        cam->projection = CAMERA_PERSPECTIVE;             // Camera projection type

        shakeDuration = 0;
        shakeIntensity = 0;
        shakePos = cam->position;
        shaking = false;
    }

    void shake(float duration, float intensity)
    {
        if(shakeDuration > 0){return;}
        shakeDuration = duration;
        shakeIntensity = intensity;
        shaking = true;
        shakePos = cam->position;
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

        //shake
        if(shakeDuration > 0 && shaking)
        {
            cam->position.x += GetRandomValue(-1, 1)*shakeIntensity;
            cam->position.y += GetRandomValue(-1, 1)*shakeIntensity;
            cam->position.z += GetRandomValue(-1, 1)*shakeIntensity;
            shakeDuration -= 1.0f/60.0f;
        }
        if(shakeDuration <= 0 && shaking)
        {
            shaking = false;
            shakeDuration = 0;
            cam->position = shakePos;
        }
    }

    void setTarget(float x, float y, float z)
    {
        cam->target = (Vector3){x, y, z};
    }

    bool isShaking()
    {
        return shakeDuration > 0;
    }

    void setPos(float x, float y, float z)
    {
        if(shakeDuration > 0){return;}
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

    Vector3 getScope(int index)
    {
        return scope[index];
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
    Kapal* owner;
    std::vector<Vector3> trails;

    public:
    Bullet(Vector3 pos, Vector3 directionVec, Kapal* shooter, float speed = 0.3, float dmg = 10, float radius = 0.25) : position(pos), damage(dmg), isAlive(true)
    {
        this->speed = speed;
        this->direction = normalizeVector3(directionVec);
        this->owner = shooter;

        this->downSpeed = -1*directionVec.y;

        this->radius = radius;
    }
    ~Bullet() {}

    void update();

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
        health = 50;
    }

    float getHealth()
    {
        return health;
    }

    Vector3 getLocalAxis(int index)
    {
        return localAxis[index];
    }

    void shoot(bool right, Vector3 direction)
    {
        if(right && cooldownTimer_R > 0) {return;}
        else if(!right && cooldownTimer_L > 0) {return;}

        if(right) {cooldownTimer_R = cooldown;}
        else {cooldownTimer_L = cooldown;}

        Vector3 bulletPos = position;
        bulletPos.y += 0.5;
        bulletPos.z -= 1.5*cos((angle)*DEG2RAD);
        bulletPos.x -= 1.5*sin((angle)*DEG2RAD);

        // Vector3 bulletDir;
        // bulletDir.x = sin((angle - 90)*DEG2RAD);
        // bulletDir.z = cos((angle - 90)*DEG2RAD);
        // bulletDir.y = sin(tempRoll*DEG2RAD);

        // if(!right)
        // {
        //     bulletDir.x *= -1;
        //     bulletDir.y *= -1;
        //     bulletDir.z *= -1;
        // }

        Bullet* temp = new Bullet(bulletPos, direction, this);
        Bullets.push_back(temp);
    }

    void draw()
    {
        DrawModel(model, {position.x, position.y + 1.5f, position.z}, scale, WHITE);
        DrawCube({position.x, position.y + 2, position.z}, 0.25, 0.25, 2*(health/50), RED);
    }

    void debugDraw()
    {
        DrawBoundingBox(hitboxes.ship, RED);
        DrawCube(hitboxes.ship.min, 0.5, 0.5, 0.5, RED);
        DrawCube(hitboxes.ship.max, 0.5, 0.5, 0.5, RED);
    }

    void updateBoundingBox()
    {
        hitboxes.ship.min.x = position.x - 1 - 2*abs(sin(angle*DEG2RAD));
        hitboxes.ship.min.y = position.y - 1.5;
        hitboxes.ship.min.z = position.z - 1 - 2*abs(cos(angle*DEG2RAD));

        hitboxes.ship.max.x = position.x + 1 + 2*abs(sin(angle*DEG2RAD));
        hitboxes.ship.max.y = position.y + 1.5;
        hitboxes.ship.max.z = position.z + 1 + 2*abs(cos(angle*DEG2RAD));
    }

    Vector3 getPos()
    {
        return position;
    }
    
    virtual MyCam* getCam() {return nullptr;}

    float getAngle()
    {
        return angle;
    }

    ~Kapal()
    {
        UnloadModel(model);
    }
};

void Bullet::update()
{
    position.x += direction.x * speed;
    position.z += direction.z * speed;

    position.y -= downSpeed;
    downSpeed += GRAVITY/60;

    if(position.y < -1) {isAlive = false; return;}

    //check collision
    for(int i = 0; i < ShipHitboxes.size(); i++)
    {
        if(CheckCollisionBoxSphere(ShipHitboxes[i]->ship, position, radius) && ShipHitboxes[i]->owner != nullptr && ShipHitboxes[i]->owner != this->owner)
        {
            *(ShipHitboxes[i]->health) -= damage;
            if(i == 0)
            {
                ShipHitboxes[i]->owner->getCam()->shake(0.5, 0.5);
            }
            isAlive = false;
        }
    }

}

class MKapal : public Kapal
{
private:

    float dist_cam2kapal;
    MyCam* camera;
    std::vector<Kapal*>& enemies;


public:
    MKapal(Vector3 pos, float initAngle, MyCam* cam, std::vector<Kapal*>& enemiesArray) : Kapal(pos), enemies(enemiesArray)
    {
        camera = cam;

        // this->enemies = enemiesArray;

        hitboxes.health = &health;
        updateBoundingBox();
        hitboxes.owner = this;
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

    void restart()
    {
        position = {0, 0, 0};
        cooldownTimer_L = 0;
        cooldownTimer_R = 0;

        health = 50;
        angle = 90;

        camera->setTarget(0, 0, 0);
        camera->setPos(-10.0f, 10, 0);
    }

    MyCam* getCam() override
    {
        return camera;
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
        
        if(((dist_cam2kapal > 6 && zoomMove > 0) || (dist_cam2kapal < 100 && zoomMove < 0)) && !camera->isShaking())
        {
            CameraMoveForward(camera->getCam(), zoomMove, false);
        }
        camera->setTarget(position.x, 0, position.z);
        determineLocalAxis();

        //shoot
        if(IsKeyReleased(KEY_RIGHT))
        {
            //default bullet direction
            Vector3 bulletDir;
            bulletDir.x = sin((angle - 90)*DEG2RAD);
            bulletDir.z = cos((angle - 90)*DEG2RAD);
            bulletDir.y = sin(tempRoll*DEG2RAD);

            shoot(true, bulletDir);
        }
        if(IsKeyReleased(KEY_LEFT))
        {
            //default bullet direction
            Vector3 bulletDir;
            bulletDir.x = -1*sin((angle - 90)*DEG2RAD);
            bulletDir.z = -1*cos((angle - 90)*DEG2RAD);
            bulletDir.y = -1*sin(tempRoll*DEG2RAD);

            shoot(false, bulletDir);
        }

        if(cooldownTimer_R > 0) {cooldownTimer_R--;}
        if(cooldownTimer_L > 0) {cooldownTimer_L--;}
        
        updateBoundingBox();
    }
};

class EKapal : public Kapal
{
    private:
    Kapal* target;
    float angleToFace;
    bool active;

    std::vector<bool> control()
    {
        const float minDistToAttack = 20.0f;
        const float angleTolerance = 5;
        std::vector<bool> movement = {false,  //W 
                                      false,  //A
                                      false,  //S
                                      false,  //D
                                      false,  //shoot right
                                      false}; //shoot left

        angleToFace = atan2f(position.x - target->getPos().x, position.z - target->getPos().z)*RAD2DEG + 180;

        float angleBetween = this->angleToFace - this->angle;
        if(Vector3Distance(target->getPos(), this->position) >= minDistToAttack)
        {
            movement[0] = true;
            if(angleBetween <= 180 && abs(angleBetween) > angleTolerance && angleBetween > 0)
            {
                movement[1] = true;
            }
            else if((angleBetween > 180 || angleBetween < 0) && abs(angleBetween) > 5)
            {
                movement[3] = !movement[1];
            }
        }
        else
        {
            //combat
            float halAngle = angle - target->getAngle();
            if(halAngle < 0) {halAngle += 360;}
            if(halAngle > 360) {halAngle -= 360;}
            movement[0] = true;
            if(halAngle < 180 && !(Vector3Angle(localAxis[2] , normalizeVector3(Vector3Subtract(target->getPos(), position))) * RAD2DEG < 5 || Vector3Angle(Vector3Scale(localAxis[2], -1) , normalizeVector3(Vector3Subtract(target->getPos(), position))) * RAD2DEG < 5))
            {
                if(halAngle > 90)
                {
                    movement[1] = true;
                }
                else
                {
                    movement[3] = true;
                }
            }
            else if(halAngle > 180 && !(Vector3Angle(localAxis[2] , normalizeVector3(Vector3Subtract(target->getPos(), position))) * RAD2DEG < 5 || Vector3Angle(Vector3Scale(localAxis[2], -1) , normalizeVector3(Vector3Subtract(target->getPos(), position))) * RAD2DEG < 5))
            {
                if(halAngle < 270)
                {
                    movement[3] = true;
                }
                else
                {
                    movement[1] = true;
                }
            }
        }

        float aimAngle = Vector3Angle(localAxis[2] , normalizeVector3(Vector3Subtract(target->getPos(), position))) * RAD2DEG;
        if(0 < aimAngle && aimAngle < 10)
        {
            movement[4] = true;
        }
        aimAngle = Vector3Angle(Vector3Scale(localAxis[2], -1) , normalizeVector3(Vector3Subtract(target->getPos(), position))) * RAD2DEG < 10;
        if(0 < aimAngle && aimAngle < 10)
        {
            movement[5] = true;
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
        if(movement[4])
        {
            Vector3 bulletDir;
            bulletDir.x = sin((angle - 90)*DEG2RAD);
            bulletDir.z = cos((angle - 90)*DEG2RAD);
            bulletDir.y = sin(tempRoll*DEG2RAD);
            shoot(true, bulletDir);
        }
        if(movement[5])
        {
            Vector3 bulletDir;
            bulletDir.x = -1*sin((angle - 90)*DEG2RAD);
            bulletDir.z = -1*cos((angle - 90)*DEG2RAD);
            bulletDir.y = -1*sin(tempRoll*DEG2RAD);
            shoot(false, bulletDir);
        }

        if(cooldownTimer_R > 0) {cooldownTimer_R--;}
        if(cooldownTimer_L > 0) {cooldownTimer_L--;}

        updateBoundingBox();
    }

    void restart(Vector3 pos)
    {
        position = pos;
        health = 50;
    }

    bool isActive()
    {
        return active;
    }

    void setActive(bool act, Vector3 pos = {0, 0, 0})
    {
        if(act)
        {
            active = true;
            restart(pos);
        }
        else
        {
            active = false;
        }
    }

    EKapal(Vector3 pos, float initAngle, Kapal* target) : Kapal(pos)
    {
        this->target = target;
        scale = 0.25f;
        angle = 90 + initAngle;
        active = false;

        hitboxes.health = &health;
        updateBoundingBox();
        hitboxes.owner = this;
        ShipHitboxes.push_back(&hitboxes);

        throttle = 0;
        tempRoll = 0;

        buoyancyPeriod = 0;
        buoyancyAngle = 0;

        localAxis[0] = {1, 0, 0};
        localAxis[1] = {0, 1, 0};   
        localAxis[2] = {0, 0, 1};
    }

    ~EKapal()
    {
        UnloadTexture(model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture);
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

Vector3 normalizeVector3(Vector3 v)
{
    float length = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    return {v.x/length, v.y/length, v.z/length};
}

class Explosion
{
    private:
    Vector3 pos;
    float minRadius;
    float maxRadius;
    float radius;
    float time;
    Color color;
    bool active;

    public:
    Explosion(Vector3 Pos, float startRadius = 1, float maxRadius = 3, Color color = RED, float Time = 0.5) : pos(Pos), radius(startRadius), color(color), time(Time)
    {
        this->minRadius = startRadius;
        this->maxRadius = maxRadius;
        active = true;
    }

    bool isActive()
    {
        return active;
    }

    void update()
    {
        radius += (maxRadius - minRadius)/(60*time);
        if(radius >= maxRadius) {active = false;}
    }

    void draw()
    {
        DrawSphere(pos, radius, color);
    }
};


std::vector<Kapal*> enemyKapals_copy;
std::vector<EKapal*> enemyKapals;
std::vector<Explosion*> explosions;
void createEnemyKapal(Vector3 pos, float angle, Kapal* target)
{
    EKapal* temp = new EKapal(pos, angle, target);
    enemyKapals.push_back(temp);
    enemyKapals_copy.push_back(temp);
}

Vector3 getRandomPos(Vector3 center, float radius, bool inside)
{
    Vector3 temp;
    temp.y = 0;

    float r;
    float theta = GetRandomValue(0, 360)*DEG2RAD;

    if(inside)
    {
        r = GetRandomValue(0, radius);
    }
    else
    {
        r = GetRandomValue(radius, 10);
    }

    temp.x = center.x + r*sin(theta);
    temp.z = center.z + r*cos(theta);
    return temp;
}

class Button
{
    private:
    Rectangle inner;
    Rectangle outer;

    Color color_main;
    Color color_border;
    Color color_hover;
    Color color_text;

    const char* name;

    int textSize;

    bool hover;

    public:
    Button(Vector2 pos, float width, float height , const char* name, int fontSize, float borderLen = 10, Color color = WHITE, Color borderColor = BLACK, Color hoverColor = GRAY, Color textColor = BLACK)
    {
        inner.x = pos.x;
        inner.y = pos.y;
        outer.x = pos.x - 10;
        outer.y = pos.y - 10;

        inner.width = width;
        inner.height = height;
        outer.width = width + 20;
        outer.height = height + 20;

        this->color_main = color;
        this->color_border = borderColor;
        this->color_hover = hoverColor;
        this->color_text = textColor;

        this->name = name;

        this->textSize = fontSize;
    }

    bool update()
    {
        if(CheckCollisionPointRec(GetMousePosition(), inner))
        {
            hover = true;
            if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            {
                return true;
            }
        }
        else
        {
            hover = false;
        }

        return false;
    }

    void draw()
    {
        DrawRectangleRec(outer, color_border);
        if(hover)
        {
            DrawRectangleRec(inner, color_hover);
        }
        else
        {
            DrawRectangleRec(inner, color_main);
        }

        DrawText(name, inner.x + (inner.width/2.0f) - textSize*TextLength(name)/4, inner.y + inner.height/2 - textSize/2, textSize, color_text);
        // DrawText(name, 0, inner.y + inner.height/2 - textSize/2, textSize, color_text);
    }
};

class CheckBox
{
    private:
    Color color_border;
    Color color_hover; 
    Color color_inside;
    Color color_char; 

    Rectangle main;
    Rectangle border;

    bool* value;
    bool hover;

    public:
        CheckBox(Rectangle rec, bool* value, Color borderColor = BLACK, Color main = WHITE, Color charColor = BLACK, Color hover = GRAY) : main(rec), color_border(borderColor), color_char(charColor), color_inside(main), color_hover(hover)
    {
        this->value = value;
        border.x = rec.x - 10;
        border.y = rec.y - 10;

        border.width = rec.width + 20;
        border.height = rec.height + 20;
    }

    void update()
    {
        if(CheckCollisionPointRec(GetMousePosition(), main))
        {
            hover = true;
            if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            {
                *value = !(*value);
            }
        }
        else
        {
            hover = false;
        }
    }

    void draw()
    {
        DrawRectangleRec(border, color_border);
        if(hover)
        {
            DrawRectangleRec(main, color_hover);
        }
        else
        {
            DrawRectangleRec(main, color_inside);
        }

        if(*value)
        {
            DrawText("x", main.x + main.width/4 - TextLength("x")/2, main.y, main.height, color_char);
        }
    }
};

void logoScreen(int framesCounter)
{
    int logoPositionX;
    int logoPositionY;

    int lettersCount = 0;

    int topSideRecWidth, initialTopSideRecWidth;
    int leftSideRecHeight, initialLeftSideRecHeight;
    int factor = 0;

    int bottomSideRecWidth, initialBottomSideRecWidth;
    int rightSideRecHeight, initialRightSideRecHeight;

    int state = 0;
    float alpha = 1.0f;

    bool skipTitle = 0;

    while (!skipTitle && state < 4)
    {
        if (IsKeyReleased(KEY_SPACE)) {break;}

        logoPositionX = GetScreenWidth()*4/10;
        logoPositionY = GetScreenHeight()*4/10 - GetScreenWidth()/80 ;

        initialTopSideRecWidth = GetScreenWidth()/80;
        initialLeftSideRecHeight = GetScreenWidth()/80;

        initialBottomSideRecWidth = GetScreenWidth()/80;
        initialRightSideRecHeight = GetScreenWidth()/80;

        if (state == 0)
        {
            framesCounter++;

            if (framesCounter == 120)
            {
                state = 1;
                framesCounter = 0;
            }
        }
        else if (state == 1)
        {
            factor += 4;
            topSideRecWidth = initialTopSideRecWidth + GetScreenWidth()*factor/1280;
            leftSideRecHeight = initialLeftSideRecHeight + GetScreenWidth()*factor/1280;

            if (topSideRecWidth >= GetScreenWidth()/5) {factor = 0; state = 2;}
        }
        else if (state == 2)
        {
            factor += 4;
            bottomSideRecWidth = initialBottomSideRecWidth + GetScreenWidth()*factor/1280;
            rightSideRecHeight = initialRightSideRecHeight + GetScreenWidth()*factor/1280;

            if (bottomSideRecWidth >= GetScreenWidth()/5) state = 3;
        }
        else if (state == 3)
        {
            framesCounter++;

            if (framesCounter/12)
            {
                lettersCount++;
                framesCounter = 0;
            }

            if (lettersCount >= 10)
            {
                alpha -= 0.02f;

                if (alpha <= 0.0f)
                {
                    alpha = 0.0f;
                    state = 4;
                }
            }
        }
        else if (state == 4)
        {
            break;
        }

        BeginDrawing();

            ClearBackground(RAYWHITE);

            if (state == 0)
            {
                if ((framesCounter/15)%2) DrawRectangle(logoPositionX, logoPositionY, GetScreenWidth()/80, GetScreenWidth()/80, BLACK);
            }
            else if (state == 1)
            {
                DrawRectangle(logoPositionX, logoPositionY, topSideRecWidth, GetScreenWidth()/80, BLACK);
                DrawRectangle(logoPositionX, logoPositionY, GetScreenWidth()/80, leftSideRecHeight, BLACK);
            }
            else if (state == 2)
            {
                DrawRectangle(logoPositionX, logoPositionY, GetScreenWidth()/5, GetScreenWidth()/80, BLACK);
                DrawRectangle(logoPositionX, logoPositionY, GetScreenWidth()/80, GetScreenWidth()/5, BLACK);

                DrawRectangle(logoPositionX + GetScreenWidth()*3/16, logoPositionY, GetScreenWidth()/80, rightSideRecHeight, BLACK);
                DrawRectangle(logoPositionX, logoPositionY + GetScreenWidth()*3/16, bottomSideRecWidth, GetScreenWidth()/80, BLACK);
            }
            else if (state == 3)
            {
                DrawRectangle(logoPositionX, logoPositionY, GetScreenWidth()/5, GetScreenWidth()/80, Fade(BLACK, alpha));
                DrawRectangle(logoPositionX, logoPositionY, GetScreenWidth()/80, GetScreenWidth()/5, Fade(BLACK, alpha));

                DrawRectangle(logoPositionX + GetScreenWidth()*3/16, logoPositionY, GetScreenWidth()/80, GetScreenWidth()/5, Fade(BLACK, alpha));
                DrawRectangle(logoPositionX, logoPositionY + GetScreenWidth()*3/16, GetScreenWidth()/5, GetScreenWidth()/80, Fade(BLACK, alpha));

                DrawText(TextSubtext("raylib", 0, lettersCount), logoPositionX + GetScreenWidth()*3/16 - MeasureText("raylib", GetScreenWidth()*5/128) - GetScreenWidth()/64, logoPositionY + GetScreenWidth()*3/16 - GetScreenWidth()*5/128 - GetScreenWidth()/64, GetScreenWidth()*5/128, Fade(BLACK, alpha));
            }

        EndDrawing();
        
    }
}

void nameScreen(int framesCounter)
{
    Vector2 namePosition;
    Vector2 nrpPosition;
    float alpha = 0.0f;
    bool skipName = 0;
    int nameSize;
    int nrpSize;


        while (!skipName && framesCounter <= 150)
        {
            BeginDrawing();
            if(IsKeyDown(KEY_SPACE)) {break;}
            nameSize = GetScreenWidth() * 5/128;
            nrpSize = GetScreenWidth() * 5/256;
            namePosition = {(float)(GetScreenWidth()/2 - MeasureText("FARREL GANENDRA", nameSize)/2), (float)(GetScreenHeight()/2 - nameSize - GetScreenHeight()/160)};
            nrpPosition = {(float)(GetScreenWidth()/2 - MeasureText("5024231036", nrpSize)/2), (float)(GetScreenHeight()/2 + GetScreenHeight()/160)};

            if(framesCounter <= 30)
            {
                ClearBackground(RAYWHITE);
                DrawText("FARREL GANENDRA", (int)namePosition.x, (int)namePosition.y, nameSize, Fade(BLACK, alpha));
                DrawText("5024231036", (int)nrpPosition.x, (int)nrpPosition.y, nrpSize, Fade(BLACK, alpha));
                alpha += 0.033f;
            }
            else if(framesCounter > 30 && framesCounter <= 120)
            {
                ClearBackground(RAYWHITE);
                DrawText("FARREL GANENDRA", (int)namePosition.x, (int)namePosition.y, nameSize, BLACK);
                DrawText("5024231036", (int)nrpPosition.x, (int)nrpPosition.y, nrpSize, BLACK);
            }
            else if(framesCounter > 120 && framesCounter <= 150)
            {
                ClearBackground(RAYWHITE);
                DrawText("FARREL GANENDRA", (int)namePosition.x, (int)namePosition.y, nameSize, Fade(BLACK, alpha));
                DrawText("5024231036", (int)nrpPosition.x, (int)nrpPosition.y, nrpSize, Fade(BLACK, alpha));
                alpha -= 0.033f;
            }

            framesCounter++;
            EndDrawing();
        }
    
}

int main(void)
{
    InitWindow(screenWidth, screenHeight, "KAPAL");

    bool debug = false;

    MyCam camera({0, 0, 0});
    
    MKapal main_kapal({0, 1.5, 0}, 0, &camera, enemyKapals_copy);

    const int maxEnemy = 11;
    int startingEnemy = 2;
    int activeEnemy = startingEnemy;
    for(int i = 0; i < maxEnemy; i++)
    {
        // createEnemyKapal(getRandomPos(main_kapal.getPos(), Vector3Distance(main_kapal.getPos(), ocean.getScope(1)), false), GetRandomValue(0, 360), &main_kapal);
        createEnemyKapal(getRandomPos(main_kapal.getPos(), 23.67379f, false), GetRandomValue(0, 360), &main_kapal);
    }

    for(int i = 0; i < startingEnemy; i++)
    {
        enemyKapals[i]->setActive(true, getRandomPos(main_kapal.getPos(), 23.67379f, false));
    }

    Ocean ocean(100, &camera, 0.01, 0.025);
    int gamestate = MENU;

    ToggleFullscreen();
    SetTargetFPS(60);

    logoScreen(frameCounter);
    nameScreen(frameCounter);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        
        switch (gamestate)
        {
        case MENU:
        {
            ClearBackground(SEABLUE);

            BeginMode3D(*(camera.getCam()));
                ocean.update();
                ocean.drawWaves();
            EndMode3D();

            activeEnemy = startingEnemy;
            for(int i = 0; i < maxEnemy; i++)
            {
                if(i < startingEnemy)
                {
                    enemyKapals[i]->setActive(true, getRandomPos(main_kapal.getPos(), 23.67379f, false));
                }
                else
                {
                    enemyKapals[i]->setActive(false, {0, 0, 0});
                }
            }

            Button playButton({(float)GetScreenWidth()/2.0f - 300, (float)GetScreenHeight()/2.0f - 100.0f}, 600, 200, "Play!", 100);
            Button settingButton({(float)GetScreenWidth()/2.0f - 300.0f, (float)GetScreenHeight()/2.0f + 130.0f}, 285, 100, "Settings", 50);
            Button exitButton({(float)GetScreenWidth()/2.0f + 15, (float)GetScreenHeight()/2.0f + 130.0f}, 285, 100, "Exit", 50);

            if(playButton.update())
            {
                main_kapal.restart();
                gamestate = GAMEPLAY;
            }
            else if(settingButton.update()) {gamestate = SETTING;}
            else if(exitButton.update()) {CloseWindow();}
            
            playButton.draw();
            settingButton.draw();
            exitButton.draw();

        }break;
        case SETTING:
        {
            ClearBackground(SEABLUE);

            BeginMode3D(*(camera.getCam()));
                ocean.update();
                ocean.drawWaves();
            EndMode3D();

            Button playButton({(float)GetScreenWidth()/2.0f - 300, (float)GetScreenHeight()/2.0f - 100.0f}, 600, 200, "Play!", 100);
            Button settingButton({(float)GetScreenWidth()/2.0f - 300.0f, (float)GetScreenHeight()/2.0f + 130.0f}, 285, 100, "Settings", 50);
            Button exitButton({(float)GetScreenWidth()/2.0f + 15, (float)GetScreenHeight()/2.0f + 130.0f}, 285, 100, "Exit", 50);

            playButton.draw();
            settingButton.draw();
            exitButton.draw();

            DrawRectangle((float)GetScreenWidth()/2.0f - 260, (float)GetScreenHeight()/2.0f - 510, 520, 1020, BLACK);
            DrawRectangle((float)GetScreenWidth()/2.0f - 250, (float)GetScreenHeight()/2.0f - 500, 500, 1000, WHITE);

            Rectangle closeSetting = {(float)GetScreenWidth()/2.0f + 190, (float)GetScreenHeight()/2.0f - 490, 50, 50};
            if(CheckCollisionPointRec(GetMousePosition(), closeSetting))
            {
                DrawRectangleRec(closeSetting, GRAY);
                if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {gamestate = MENU;}
            }
            DrawText("x", (int)closeSetting.x + closeSetting.width/4 - TextLength("x")/2, (int)closeSetting.y + closeSetting.width/2 - 25, 50, BLACK);

            CheckBox debugCheckBox({(float)GetScreenWidth()/2 - 230, (float)GetScreenHeight()/2 - 420, 50, 50}, &debug);
            debugCheckBox.update();
            debugCheckBox.draw();

            DrawText("Debug mode", GetScreenWidth()/2 - 160, GetScreenHeight()/2 - 420, 50, BLACK);

        }break;
        case GAMEPLAY:
            ClearBackground(SEABLUE);
            if(IsKeyReleased(KEY_P)) {gamestate = PAUSE;}
            if(main_kapal.getHealth() <= 0) {gamestate = DEAD;}

            BeginMode3D(*(camera.getCam()));
                //game update
                main_kapal.move();

                for(int i = 0; i < enemyKapals.size(); i++)
                {
                    if(!enemyKapals[i]->isActive()) {break;}
                    enemyKapals[i]->move();
                    if(enemyKapals[i]->getHealth() <= 0)
                    {
                        Explosion* tempExplosion = new Explosion(enemyKapals[i]->getPos());
                        explosions.push_back(tempExplosion);
                        enemyKapals[i]->restart(getRandomPos(main_kapal.getPos(), Vector3Distance(main_kapal.getPos(), ocean.getScope(1)), false));
                        if(activeEnemy < maxEnemy)
                        {
                            enemyKapals[activeEnemy]->setActive(true, getRandomPos(main_kapal.getPos(), Vector3Distance(main_kapal.getPos(), ocean.getScope(1)), false));
                            activeEnemy++;
                        }
                    }
                }

                for(int i = 0; i < explosions.size(); i++)
                {
                    if(!explosions[i]->isActive())
                    {
                        delete explosions[i];
                        explosions.erase(explosions.begin() + i);
                        // i--;
                    }
                    explosions[i]->update();
                }

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
                for(int i = 0; i < Bullets.size(); i++)
                {
                    Bullets[i]->draw();
                }
                
                main_kapal.draw();

                for(int i = 0; i < enemyKapals.size(); i++)
                {
                    if(!enemyKapals[i]->isActive()){break;}
                    enemyKapals[i]->draw();
                }

                for(int i = 0; i < explosions.size(); i++)
                {
                    explosions[i]->draw();
                }

                ocean.drawWaves();
                
                //debug draw
                if(debug)
                {
                    DrawGrid(1000, 1);
                    main_kapal.debugDraw();
                    for(int i = 0; i < enemyKapals.size(); i++)
                    {
                        enemyKapals[i]->debugDraw();
                    }
                }

            EndMode3D();
            
            if(debug)
            {
                DrawText(TextFormat("mainship angle: %f", main_kapal.getAngle()), 10, 10, 40, RED);
            }
            break;
        case PAUSE:
        {
            ClearBackground(SEABLUE);
            BeginMode3D(*(camera.getCam()));
            for(int i = 0; i < Bullets.size(); i++)
            {
                Bullets[i]->draw();
            }
                
            main_kapal.draw();

            for(int i = 0; i < enemyKapals.size(); i++)
            {
                if(!enemyKapals[i]->isActive()){break;}
                enemyKapals[i]->draw();
            }

            ocean.drawWaves();
            
            for(int i = 0; i < explosions.size(); i++)
            {
                explosions[i]->draw();
            }
                
            //debug draw
            if(debug)
            {
                DrawGrid(1000, 1);
                main_kapal.debugDraw();
                for(int i = 0; i < enemyKapals.size(); i++)
                {
                    enemyKapals[i]->debugDraw();
                }
            }

            EndMode3D();
            
            if(debug)
            {
                DrawText(TextFormat("mainship angle: %f", main_kapal.getAngle()), 10, 10, 40, RED);
            }

            DrawRectangle((float)GetScreenWidth()/2.0f - 410, (float)GetScreenHeight()/2.0f - 310, 820, 470, BLACK);
            DrawRectangle((float)GetScreenWidth()/2.0f - 400, (float)GetScreenHeight()/2.0f - 300, 800, 450, WHITE);

            DrawText("Paused", GetScreenWidth()/2 - 25*TextLength("You Dead"), (float)GetScreenHeight()/2.0f - 200, 100, BLACK);

            Button menuButton({(float)GetScreenWidth()/2.0f - 300.0f, (float)GetScreenHeight()/2.0f - 50}, 285, 100, "Menu", 50);
            Button retryButton({(float)GetScreenWidth()/2.0f + 15, (float)GetScreenHeight()/2.0f - 50}, 285, 100, "Continue", 50);

            if(menuButton.update()) {gamestate = MENU;}
            else if(retryButton.update()) {gamestate = GAMEPLAY;}
            
            menuButton.draw();
            retryButton.draw();
        }break;
        case DEAD:
        {
            ClearBackground(SEABLUE);
            BeginMode3D(*(camera.getCam()));
            for(int i = 0; i < Bullets.size(); i++)
            {
                Bullets[i]->draw();
            }
                
            // main_kapal.draw();

            for(int i = 0; i < enemyKapals.size(); i++)
            {
                if(!enemyKapals[i]->isActive()){break;}
                enemyKapals[i]->draw();
            }

            ocean.drawWaves();
            
            for(int i = 0; i < explosions.size(); i++)
            {
                explosions[i]->draw();
            }
                
            //debug draw
            if(debug)
            {
                DrawGrid(1000, 1);
                main_kapal.debugDraw();
                for(int i = 0; i < enemyKapals.size(); i++)
                {
                    enemyKapals[i]->debugDraw();
                }
            }

            EndMode3D();
            
            if(debug)
            {
                DrawText(TextFormat("mainship angle: %f", main_kapal.getAngle()), 10, 10, 40, RED);
            }

            DrawRectangle((float)GetScreenWidth()/2.0f - 410, (float)GetScreenHeight()/2.0f - 310, 820, 470, BLACK);
            DrawRectangle((float)GetScreenWidth()/2.0f - 400, (float)GetScreenHeight()/2.0f - 300, 800, 450, WHITE);

            DrawText("You Dead", GetScreenWidth()/2 - 25*TextLength("You Dead") - 25, (float)GetScreenHeight()/2.0f - 200, 100, BLACK);

            Button menuButton({(float)GetScreenWidth()/2.0f - 300.0f, (float)GetScreenHeight()/2.0f - 50}, 285, 100, "Menu", 50);
            Button retryButton({(float)GetScreenWidth()/2.0f + 15, (float)GetScreenHeight()/2.0f - 50}, 285, 100, "Retry", 50);

            if(menuButton.update()) {gamestate = MENU;}
            else if(retryButton.update()) 
            {
                for(int i = 0; i < maxEnemy; i++)
                {
                    if(i < startingEnemy)
                    {
                        enemyKapals[i]->setActive(true, getRandomPos(main_kapal.getPos(), 23.67379f, false));
                    }
                    else
                    {
                        enemyKapals[i]->setActive(false, {0, 0, 0});
                    }
                }
                main_kapal.restart();
                gamestate = GAMEPLAY;
            }
            
            menuButton.draw();
            retryButton.draw();

        }break;
        }
        EndDrawing();

        frameCounter++;
    }
    CloseWindow();
    return 0;
}

//Farrel Ganendra | 06 - Juli - 2024