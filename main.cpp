#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <btBulletDynamicsCommon.h>
#include <vector>
#include <memory>
#include <cstdlib>
#include <ctime>

struct RigidBodyData {
    btRigidBody* body;
    enum Type { BOX, SPHERE } type;
    Color color;
};

std::vector<RigidBodyData> objects;

btDiscreteDynamicsWorld* world = nullptr;

void AddRandomObject(btDiscreteDynamicsWorld* world) {
    float x = ((float)(rand() % 10) - 5.0f);
    float z = ((float)(rand() % 10) - 5.0f);
    float y = 10.0f;

    bool isSphere = rand() % 2;

    btCollisionShape* shape = isSphere ?
        static_cast<btCollisionShape*>(new btSphereShape(0.5f)) :
        static_cast<btCollisionShape*>(new btBoxShape(btVector3(0.5f, 0.5f, 0.5f)));

    // Losowa rotacja (Euler XYZ → Quaternion)
    float angleX = ((float)(rand() % 360)) * DEG2RAD;
    float angleY = ((float)(rand() % 360)) * DEG2RAD;
    float angleZ = ((float)(rand() % 360)) * DEG2RAD;

    Quaternion q = QuaternionFromEuler(angleX, angleY, angleZ);
    btQuaternion btQ(q.x, q.y, q.z, q.w);

    btDefaultMotionState* motionState = new btDefaultMotionState(
        btTransform(btQ, btVector3(x, y, z))
    );

    btScalar mass = 1.0f;
    btVector3 inertia(0, 0, 0);
    shape->calculateLocalInertia(mass, inertia);

    btRigidBody::btRigidBodyConstructionInfo info(mass, motionState, shape, inertia);
    btRigidBody* body = new btRigidBody(info);
    world->addRigidBody(body);

    // Losowy kolor
    Color c = Color{
        (unsigned char)(100 + rand() % 156),
        (unsigned char)(100 + rand() % 156),
        (unsigned char)(100 + rand() % 156),
        255
    };

    objects.push_back({body, isSphere ? RigidBodyData::SPHERE : RigidBodyData::BOX, c});
}

int main() {
    srand(time(nullptr));

    InitWindow(1280, 720, "raylib + Bullet demo");

    Shader lighting = LoadShader("shaders/lighting.vs", "shaders/lighting.fs");

    // Setup uniform locations
    int viewPosLoc = GetShaderLocation(lighting, "viewPos");
    int lightPosLoc = GetShaderLocation(lighting, "lightPos");
    int lightColorLoc = GetShaderLocation(lighting, "lightColor");

    Model cubeModel = LoadModelFromMesh(GenMeshCube(1, 1, 1));
    cubeModel.materials[0].shader = lighting;

    Model sphereModel = LoadModelFromMesh(GenMeshSphere(0.5f, 16, 16));
    sphereModel.materials[0].shader = lighting;

    SetTargetFPS(60);

    //Lights

    Vector3 lightPos[5] = {
        { 5, 10, 5 },
        {-5, 10, 5 },
        { 5, 10, -5 },
        {-5, 10, -5 },
        { 0, 12,  0 }
    };

    Color lightColors[5] = {
        RED, GREEN, BLUE, YELLOW, PURPLE
    };

    // Raz na start:
    Vector3 lightColorVecs[5];
    for (int i = 0; i < 5; i++) {
        lightColorVecs[i] = Vector3{
            lightColors[i].r / 255.0f,
            lightColors[i].g / 255.0f,
            lightColors[i].b / 255.0f
        };
    }


    Camera3D camera = { 0 };
    camera.position = Vector3{ 10.0f, 10.0f, 10.0f };
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Bullet physics setup
    btBroadphaseInterface* broadphase = new btDbvtBroadphase();
    btDefaultCollisionConfiguration* collisionConfig = new btDefaultCollisionConfiguration();
    btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfig);
    btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;

    world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfig);
    world->setGravity(btVector3(0, -9.81f, 0));

    // Ground plane
    btCollisionShape* groundShape = new btBoxShape(btVector3(20.0f, 0.5f, 20.0f));
    btDefaultMotionState* groundMotionState = new btDefaultMotionState(
        btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, -0.5f, 0))
    );
    btRigidBody::btRigidBodyConstructionInfo groundInfo(0, groundMotionState, groundShape);
    btRigidBody* ground = new btRigidBody(groundInfo);
    world->addRigidBody(ground);

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE)) {
            AddRandomObject(world);
        }

        world->stepSimulation(GetFrameTime(), 10);

        //Uniform update
        Vector3 camPos = camera.position;
        SetShaderValue(lighting, viewPosLoc, &camPos, SHADER_UNIFORM_VEC3);
        SetShaderValueV(lighting, lightPosLoc, lightPos, SHADER_UNIFORM_VEC3, 5);
        SetShaderValueV(lighting, lightColorLoc, lightColorVecs, SHADER_UNIFORM_VEC3, 5);

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode3D(camera);

        DrawGrid(20, 1.0f);
        DrawCube(Vector3{0, -0.5f, 0}, 40, 1, 40, LIGHTGRAY);

        for (const auto& obj : objects) {
            btTransform trans;
            obj.body->getMotionState()->getWorldTransform(trans);
            btVector3 pos = trans.getOrigin();
            btQuaternion rot = trans.getRotation();

            Vector3 p = {pos.x(), pos.y(), pos.z()};
            Quaternion q = {rot.x(), rot.y(), rot.z(), rot.w()};

            Model& model = (obj.type == RigidBodyData::SPHERE) ? sphereModel : cubeModel;

            // Ustaw kolor dla tego obiektu
            model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = obj.color;


            /*float lightStrength = Clamp(1.0f - (p.y / 10.0f), 0.4f, 1.0f);
            Color litColor = {
                (unsigned char)(obj.color.r * lightStrength),
                (unsigned char)(obj.color.g * lightStrength),
                (unsigned char)(obj.color.b * lightStrength),
                255
            };*/

            if (obj.type == RigidBodyData::BOX) {
                rlPushMatrix();

                // Zbuduj macierz transformacji z rotacji i pozycji
                Matrix m = QuaternionToMatrix(q);
                m.m12 = p.x;
                m.m13 = p.y;
                m.m14 = p.z;

                rlMultMatrixf(MatrixToFloat(m));

                DrawCube(Vector3Zero(), 1, 1, 1, obj.color);
                //DrawModelEx(model, p, Vector3Normalize({q.x, q.y, q.z}), QuaternionLength(q) * RAD2DEG, Vector3{1,1,1}, WHITE);
                rlPopMatrix();
            } else {
                // Sfery i tak nie mają orientacji, więc zwykły DrawSphere
                DrawSphereEx(p, 0.5f, 16, 16, obj.color);
                //DrawModelEx(model, p, Vector3Normalize({q.x, q.y, q.z}), QuaternionLength(q) * RAD2DEG, Vector3{1,1,1}, WHITE);
            }
        }

        EndMode3D();

        DrawText("Press SPACE to drop a cube or sphere", 10, 10, 20, DARKGRAY);
        EndDrawing();
    }

    // Cleanup (minimal for demo)
    CloseWindow();
    return 0;
}