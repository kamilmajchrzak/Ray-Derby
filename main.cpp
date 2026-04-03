// Falling objects demo using raylib + Bullet Physics + custom lighting shader
// Requirements: raylib compiled with OpenGL 3.3 and Bullet Physics installed

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <btBulletDynamicsCommon.h>
#include <vector>
#include <cstdlib>
#include <ctime>

struct RigidBodyData {
    enum Type { BOX, SPHERE, CYLINDER, CONE, CAPSULE } type;
    btRigidBody* body;
    Color color;
    float scale;
};


std::vector<RigidBodyData> objects;
btDiscreteDynamicsWorld* world;
Model models[5];

// Random float
float RandRange(float min, float max) {
    return min + ((float)rand() / RAND_MAX) * (max - min);
}

btRigidBody* CreateRigidBody(btCollisionShape* shape, float mass, const btTransform& transform) {
    btVector3 inertia(0, 0, 0);
    if (mass != 0.0f) shape->calculateLocalInertia(mass, inertia);
    btDefaultMotionState* motionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo info(mass, motionState, shape, inertia);
    return new btRigidBody(info);
}

void AddRandomObject() {
    float scale = RandRange(0.5f, 2.0f);
    int shapeType = GetRandomValue(0, 4);
    btCollisionShape* shape = nullptr;

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(RandRange(-5, 5), 15, RandRange(-5, 5)));

    // Random rotation
    btQuaternion rotation;
    rotation.setEuler(RandRange(0, PI), RandRange(0, PI), RandRange(0, PI));
    transform.setRotation(rotation);

    RigidBodyData::Type type = static_cast<RigidBodyData::Type>(shapeType);

    switch (type) {
        case RigidBodyData::BOX:
            shape = new btBoxShape(btVector3(scale, scale, scale));
            break;
        case RigidBodyData::SPHERE:
            shape = new btSphereShape(scale);
            break;
        case RigidBodyData::CYLINDER:
            shape = new btCylinderShape(btVector3(scale, scale, scale));
            break;
        case RigidBodyData::CONE:
            shape = new btConeShape(scale, scale * 2);
            break;
        case RigidBodyData::CAPSULE:
            shape = new btCapsuleShape(scale, scale);
            break;
    }

    btRigidBody* body = CreateRigidBody(shape, 1.0f, transform);
    world->addRigidBody(body);

    Color color = ColorFromHSV(GetRandomValue(0, 360), 0.8f, 0.8f);

    objects.push_back({type, body, color, scale});
}

int main() {
    srand(time(nullptr));
    InitWindow(640, 480, "Falling Objects + Bullet + Lighting");
    //ToggleFullscreen();
    SetTargetFPS(60);

    Camera3D camera = {0};
    camera.position = { 15.0f, 15.0f, 15.0f };
    camera.target = { 0.0f, 0.0f, 0.0f };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Shader lighting = LoadShader("../shaders/lighting.vert", "../shaders/lighting.frag");


    // ️ KRYTYCZNE: Sprawdź czy shader się skompilował
    if (lighting.id == 0) {
        TraceLog(LOG_ERROR, "Shader compilation failed!");
        //CloseWindow();
        return -1;
    }

    lighting.locs[SHADER_LOC_VERTEX_POSITION] = GetShaderLocationAttrib(lighting, "vertexPosition");

    lighting.locs[SHADER_LOC_VERTEX_NORMAL] = GetShaderLocationAttrib(lighting, "vertexNormal");

    //uniforms
    int viewPosLoc = GetShaderLocation(lighting, "viewPos");
    int ambientLoc = GetShaderLocation(lighting, "ambient");
    int lightPosLoc = GetShaderLocation(lighting, "lightPos[5]");
    int lightColorLoc = GetShaderLocation(lighting, "lightColor[5]");
    int colDiffuseLoc = GetShaderLocation(lighting, "colDiffuse");

    float ambient[4] = { 0.1f, 0.1f, 0.1f, 1.0f };

    SetShaderValue(lighting, ambientLoc, ambient, SHADER_UNIFORM_VEC4);

    Vector3 lightPos[5] = {
        {10, 10, 10},
        {-10, 10, 10},
        {10, 10, -10},
        {-10, 10, -10},
        {0, 20, 0}
    };
    Vector3 lightColor[5] = {
        {1, 0.9f, 0.8f},
        {0.8f, 0.8f, 1},
        {1, 0.6f, 0.6f},
        {0.6f, 1, 0.6f},
        {0.9f, 0.9f, 1}
    };

    SetShaderValueV(lighting, lightPosLoc, &lightPos[0].x, SHADER_UNIFORM_VEC3, 5);
    SetShaderValueV(lighting, lightColorLoc, &lightColor[0].x, SHADER_UNIFORM_VEC3, 5);

    Mesh meshCube = GenMeshCube(1, 1, 1);
    models[RigidBodyData::BOX] = LoadModelFromMesh(meshCube);

    Mesh meshSphere = GenMeshSphere(1, 16, 16);
    models[RigidBodyData::SPHERE] = LoadModelFromMesh(meshSphere);

    Mesh meshCyl = GenMeshCylinder(1, 2, 16);
    models[RigidBodyData::CYLINDER] = LoadModelFromMesh(meshCyl);

    Mesh meshCone = GenMeshCone(1, 2, 16);
    models[RigidBodyData::CONE] = LoadModelFromMesh(meshCone);

    // Poprawiony generator kapsuły (wcześniej była sferą)
    Mesh meshCapsule = GenMeshSphere(1, 16, 16);
    models[RigidBodyData::CAPSULE] = LoadModelFromMesh(meshCapsule);

    // Przypisanie shadera
    for (int i = 0; i < 5; i++) {
        models[i].materials[0].shader = lighting;
        // Ważne: Ustaw domyślny kolor materiału, żeby nie był czarny
        models[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
    }


    // Physics init
    btBroadphaseInterface* broadphase = new btDbvtBroadphase();
    btDefaultCollisionConfiguration* collisionConfig = new btDefaultCollisionConfiguration();
    btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfig);
    btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver();
    world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfig);
    world->setGravity(btVector3(0, -9.81f, 0));

    btCollisionShape* groundShape = new btBoxShape(btVector3(20, 1, 20));
    btRigidBody* ground = CreateRigidBody(groundShape, 0.0f, btTransform(btQuaternion(0,0,0,1), btVector3(0, -1, 0)));
    world->addRigidBody(ground);

    TraceLog(LOG_INFO, "BOX: vertices=%d, triangles=%d", models[RigidBodyData::BOX].meshes[0].vertexCount, models[RigidBodyData::BOX].meshes[0].triangleCount);
    TraceLog(LOG_INFO, "SPHERE: vertices=%d, triangles=%d", models[RigidBodyData::SPHERE].meshes[0].vertexCount, models[RigidBodyData::SPHERE].meshes[0].triangleCount);

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE)) AddRandomObject();

        world->stepSimulation(GetFrameTime(), 10);

        float viewPos[3] = { camera.position.x, camera.position.y, camera.position.z };
        SetShaderValue(lighting, viewPosLoc, viewPos, SHADER_UNIFORM_VEC3);
        //rlEnableBackfaceCulling();
        rlDisableBackfaceCulling();
        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode3D(camera);
        DrawCube({0, -1.0f, 0}, 40, 2, 40, DARKGRAY);
        DrawGrid(40, 1.0f);

        for (const auto& obj : objects)
            {
            btTransform trans;
            obj.body->getMotionState()->getWorldTransform(trans);
            btVector3 pos = trans.getOrigin();
            btQuaternion rot = trans.getRotation();

            Vector3 p = { pos.x(), pos.y(), pos.z() };
            Quaternion q = { rot.x(), rot.y(), rot.z(), rot.w() };
            Matrix mat = QuaternionToMatrix(q);
            mat.m12 = p.x;
            mat.m13 = p.y;
            mat.m14 = p.z;

            // Przekazujemy kolor obiektu jako colDiffuse

            float colDiffuse[4] = {
                (float)obj.color.r / 255.0f,
                (float)obj.color.g / 255.0f,
                (float)obj.color.b / 255.0f,
                (float)obj.color.a / 255.0f
            };

            SetShaderValue(lighting, colDiffuseLoc, colDiffuse, SHADER_UNIFORM_VEC4);
            rlPushMatrix();
            rlMultMatrixf(MatrixToFloat(mat));
            DrawModel(models[obj.type], {0, 0, 0}, obj.scale, obj.color);
            rlPopMatrix();
        }

        EndMode3D();

        DrawText("Press SPACE to spawn random object", 10, 10, 20, RAYWHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}