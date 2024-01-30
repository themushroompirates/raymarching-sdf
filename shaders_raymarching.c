#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

#define SHADER_NODES_IMPLEMENTATION
#include "shader_nodes.h"

#include "iconset.rgi.h"
#define RAYGUI_CUSTOM_ICONS
static unsigned int *guiIconsPtr = guiIcons;

#define RAYGUI_IMPLEMENTATION
#include "external/raygui.h"

typedef enum {
    DRAG_NONE = 0,
    DRAG_ONTO,
    DRAG_BEFORE,
    DRAG_AFTER,
    DRAG_FLAG_DEBUG = 1 << 10
} DragTargetMode;

typedef struct {
    int depth;
    bool expanded;
} ShaderEditorNodeData;

typedef struct {
    int selectedIndex;
    int draggingIndex;
    Vector2 draggingOffset;
    ShaderEditorNodeData nodes[SHADER_NODES_MAX];
    bool propertiesExpanded[1024];
} ShaderEditorData;

ShaderEditorData shaderEditData = { -1, -1 };

typedef enum {
    WINDOW_VIEWPORT,
    WINDOW_HIERARCHY,
    WINDOW_PROPERTIES,
    WINDOW_SELECT_NODE_TYPE,
    WINDOW_CONFIRM_EXIT,
    WINDOW_MAX
} EditorWindowId;

Rectangle editorWindowRects[WINDOW_MAX];

EditorWindowId hotEditorWindow = WINDOW_VIEWPORT;
EditorWindowId activeEditorWindow = WINDOW_VIEWPORT;

EditorWindowId dialogStack[8];
int dialogStackCount = 0;

int activeTextboxIndex = 0;
char activeTextboxBuffer[1024];

RenderTexture2D viewportRenderTex = { 0 };

void RecalcWindowLayout();
int WindowAtPoint(Vector2 point);

void HandleWindowHierarchy();
void HandleWindowProperties();
void HandleWindowPropertiesGeneral();
void HandleWindowPropertiesObject();
void HandleWindowPropertiesOperation();
void HandleWindowPropertiesPrimitive();
void HandleWindowSelectNodeType();

void ShowAddNodeDialog();
void HideAddNodeDialog();

int HandleTextboxString(int windowId, int textBoxId, Rectangle labelBounds, char *label, Rectangle valueBounds, char *value, int indent, bool *toggler);
int HandleTextboxFloat(int windowId, int textBoxId, Rectangle labelBounds, char *label, Rectangle valueBounds, float *value, int indent);
int GuiLabelButtonEx(Rectangle bounds, const char *text, bool *active, int indent, bool *toggler);

Camera camera = { 0 };
Shader shader = { 0 };

// @TODO tidy this mess up
int viewEyeLoc;
int viewCenterLoc;
int runTimeLoc;
int resolutionLoc;
int thingyLoc;
int cameraUpLoc;
int modelViewMatrixLoc;
int modelProjMatrixLoc;

void RefreshShaderLocs();
bool RebuildShader();

bool RecalcShaderNodesHierarchy();
const char *GetShaderNodeName(int i);
const char *GetShaderNodeTypeName(ShaderNodeType type);
int GetShaderNodeTypeIcon(ShaderNodeType type);
void DrawDebugNode(int i, bool primary, float alpha, float offset);
void RecursiveSetNodeExpanded(int index, bool expanded);
void ExpandToShowNode(int index);
bool CanDragNode(int source, int target, DragTargetMode mode);
void DoDropNode(int source, int target, DragTargetMode mode);
int DuplicateNode(int nodeIndex, bool duplicateChildren);
bool DeleteNode(int nodeIndex);
int CalcNodeDescendantCount(int nodeIndex);
bool IsNodeDescendant(int child, int parent);

int GuiSpinnerFloat(Rectangle bounds, const char *text, float *value, float minValue, float maxValue, int precision, bool editMode);

void DrawSphereWires2(Vector3 centerPos, float radius, int rings, int slices, Color color);

char *formatFloat(float value);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1024;
    const int screenHeight = 768;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Raymarching Constructor");
    
    SetExitKey(KEY_NULL);
    
    RecalcWindowLayout();
    
    InitShaderWriter(&shaderWriterData);
    if (true) {
        //ShaderNode node;
        //ShaderNodePrimitive nodePrimitive = { 0 };
        //ShaderNodeOperation nodeOperation = { 0 };
        
        /*
            shaderNodes[shaderNodeCount++] = (ShaderNode){ SHADER_NODE_OBJECT };
            shaderNodes[shaderNodeCount++] = MakeShaderNodeOperation(SHADER_NODE_OP_UNION, 4, 0.1f);
                shaderNodes[shaderNodeCount++] = MakeShaderNodePrimitive(SHADER_NODE_SPHERE, (Vector3){0,0,0}, (Vector3){1});
                shaderNodes[shaderNodeCount++] = MakeShaderNodeOperation(SHADER_NODE_OP_SMOOTH, 1, 0.15f);
                    shaderNodes[shaderNodeCount++] = MakeShaderNodePrimitive(SHADER_NODE_BOX, (Vector3){0,0,0}, (Vector3){1.5f, 0.5f, 0.5f });
                shaderNodes[shaderNodeCount++] = MakeShaderNodeOperation(SHADER_NODE_OP_SMOOTH, 1, 0.15f);
                    shaderNodes[shaderNodeCount++] = MakeShaderNodePrimitive(SHADER_NODE_BOX, (Vector3){0,0,0}, (Vector3){0.5f, 1.5f, 0.5f });
                shaderNodes[shaderNodeCount++] = MakeShaderNodeOperation(SHADER_NODE_OP_SMOOTH, 1, 0.15f);
                    shaderNodes[shaderNodeCount++] = MakeShaderNodePrimitive(SHADER_NODE_BOX, (Vector3){0,0,0}, (Vector3){0.5f, 0.5f, 1.5f });
                shaderNodes[shaderNodeCount++] = MakeShaderNodePrimitive(SHADER_NODE_CYLINDER, (Vector3){-1,-1,-1}, (Vector3){1,1,1});
        ((ShaderNodePrimitive*)(&shaderNodes[9]))->radius = 0.25f;
        
        //   shaderNodes[shaderNodeCount++] = (ShaderNode){ SHADER_NODE_OBJECT };
        ((ShaderNodeObject*)(&shaderNodes[0]))->colDiffuse = LIME;
        
        shaderEditData.nodes[0].depth = 0;
        shaderEditData.nodes[1].depth = 1;
        shaderEditData.nodes[2].depth = 2;
        shaderEditData.nodes[3].depth = 2;
        shaderEditData.nodes[4].depth = 3;
        shaderEditData.nodes[5].depth = 2;
        shaderEditData.nodes[6].depth = 3;
        shaderEditData.nodes[7].depth = 2;
        shaderEditData.nodes[8].depth = 3;
        shaderEditData.nodes[9].depth = 2;
        
        shaderEditData.nodes[10].depth = 0;
        */

        shaderNodes[shaderNodeCount++] = (ShaderNode){ SHADER_NODE_OBJECT };
        ((ShaderNodeObject*)(&shaderNodes[0]))->colDiffuse = LIME;
        shaderNodes[shaderNodeCount++] = MakeShaderNodePrimitive(SHADER_NODE_BOX, (Vector3){0,0,0}, (Vector3){1.5f, 0.5f, 0.5f });
        shaderEditData.nodes[1].depth = 1;
        
        for (int i = 0; i < shaderNodeCount; i++) { TraceLog(LOG_INFO, "[Before] Node %d, child count = %d", i, shaderNodes[i].childNodeCount); }
        RecalcShaderNodesHierarchy();
        for (int i = 0; i < shaderNodeCount; i++) { TraceLog(LOG_INFO, "[After] Node %d, child count = %d, depth = %d", i, shaderNodes[i].childNodeCount, shaderEditData.nodes[i].depth); }
        
        RecalcShaderNodesNav(shaderNodes, shaderNodesNav, shaderNodeCount);
        
        TraceLog(LOG_ERROR, "--------------------------");
        for (int i = 0; i < shaderNodeCount; i++) {
            //TraceLog(LOG_ERROR, "Node #%d : descendant count = %d (ours), %d (lib's)", i, CalcNodeDescendantCount(i), shaderNodesNav[i].descendantCount);
            int j = 1;
            TraceLog(LOG_ERROR, "Node #%d descendant of node %d? %d [count = %d, delta = %d]", i, j, IsNodeDescendant(i, j), shaderNodesNav[j].descendantCount, i-j);
            //TraceLog(LOG_ERROR, "Node #%d descendant of node 9? %d", i, IsNodeDescendant(i, 9));
        }
        TraceLog(LOG_ERROR, "--------------------------");
        
        
        int byteCount = WriteShader(&shaderWriterData, shaderNodes, shaderNodeCount, 0, 0);
        if (byteCount == 0) {
            TraceLog(LOG_ERROR, "Failed to write shader");
            CloseWindow();
            return 1;
        }
        else {
            byteCount++; // For the null terminator
            
            TraceLog(LOG_INFO, "Shader takes up %d bytes", byteCount);
            char *buffer = (char*)MemAlloc(byteCount);
            byteCount = WriteShader(&shaderWriterData, shaderNodes, shaderNodeCount, buffer, byteCount);
            SaveFileText("my_shader.txt", buffer);
            MemFree(buffer);
            
            TraceLog(LOG_INFO, "Wrote %d bytes to shader file", byteCount);
        }
        
        shaderEditData.selectedIndex = 0;
    }

    camera = (Camera){ 0 };
    camera.position = (Vector3){ 2.5f, 2.5f, 3.0f };    // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.7f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    //camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.fovy = 65.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

    // Load raymarching shader
    // NOTE: Defining 0 (NULL) for vertex shader forces usage of internal default vertex shader
    //Shader shader = LoadShader(0, "raymarching2.fs");
    shader = LoadShader(0, "my_shader.txt");
    if (shader.id == rlGetShaderIdDefault()) {
        CloseWindow();
        return 1;
    }
    TraceLog(LOG_INFO, "shader id = %d", shader.id);

    // Get shader locations for required uniforms
    RefreshShaderLocs();

    float runTime = 0.0f;
    
    int showWires = 1;

    //DisableCursor();                    // Limit cursor to relative movement inside the window
    SetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())        // Detect window close button or ESC key
    {
        
        // @TODO handle modals
        hotEditorWindow = WindowAtPoint(GetMousePosition());
        
        
        // Update
        //----------------------------------------------------------------------------------

        float deltaTime = GetFrameTime();
        runTime += deltaTime;

        // Set shader required uniform values
        SetShaderValue(shader, viewEyeLoc, &camera.position, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, viewCenterLoc, &camera.target, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, runTimeLoc, &runTime, SHADER_UNIFORM_FLOAT);
        
        SetShaderValue(shader, cameraUpLoc, &camera.up, SHADER_UNIFORM_VEC3);

        // Check if screen is resized
        if (IsWindowResized())
        {
            RecalcWindowLayout();
        }
        
        if (IsKeyPressed(KEY_TAB)) {
            showWires = (showWires+1)%3;
        }
        
        if (IsKeyPressed(KEY_ESCAPE)) {
            if (activeEditorWindow == WINDOW_SELECT_NODE_TYPE) {
                HideAddNodeDialog();
            }
            else if (activeTextboxIndex > 0) {
                activeTextboxIndex = 0;
            }
            //else if (magicallyDetectDragging) {
            //  magicallyStopDragging(); // @TODO   
            //}
            else {
                
            }
        }
        
        if (hotEditorWindow == WINDOW_VIEWPORT) {
            
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                UpdateCamera(&camera, CAMERA_THIRD_PERSON);
            }
            
            // @TODO picking
            if (activeEditorWindow != WINDOW_SELECT_NODE_TYPE) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    TraceLog(LOG_INFO, "unselecting node; viewport click");
                    shaderEditData.selectedIndex = -1;
                }
            }
            
            if (IsKeyPressed(KEY_PAGE_DOWN)) {
                camera.fovy -= 10.0f * (IsKeyDown(KEY_LEFT_SHIFT) ? 0.1f : 1.0f) * (IsKeyDown(KEY_LEFT_CONTROL) ? 0.1f : 1.0f);
                float thingy = 1.0f / tan(camera.fovy * 0.5 * DEG2RAD);
                SetShaderValue(shader, thingyLoc, &thingy, SHADER_UNIFORM_FLOAT);
                
                TraceLog(LOG_INFO, "fovy / thingy :  (%.3f, %.3f)", camera.fovy, thingy);
                SetClipboardText(TextFormat("(%.3f, %.3f)", camera.fovy, thingy));
            }
            if (IsKeyPressed(KEY_PAGE_UP)) {
                camera.fovy += 10.0f * (IsKeyDown(KEY_LEFT_SHIFT) ? 0.1f : 1.0f) * (IsKeyDown(KEY_LEFT_CONTROL) ? 0.1f : 1.0f);
                float thingy = 1.0f / tan(camera.fovy * 0.5 * DEG2RAD);
                SetShaderValue(shader, thingyLoc, &thingy, SHADER_UNIFORM_FLOAT);
                
                TraceLog(LOG_INFO, "fovy / thingy :  (%.3f, %.3f)", camera.fovy, thingy);
                SetClipboardText(TextFormat("(%.3f, %.3f)", camera.fovy, thingy));
            }
            
            if (IsKeyPressed(KEY_LEFT_BRACKET)) {
                if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
                    ShaderNodePrimitive *node = (ShaderNodePrimitive*)&shaderNodes[2];
                    
                    node->size.x -= 0.1f;
                }
                else {
                    ShaderNodeOperation *node = (ShaderNodeOperation*)&shaderNodes[1];
                    
                    node->smoothness -= 0.05f;
                }
                RebuildShader(&shader);
            }
            
            if (IsKeyPressed(KEY_RIGHT_BRACKET)) {
                if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
                    ShaderNodePrimitive *node = (ShaderNodePrimitive*)&shaderNodes[2];
                
                    node->size.x += 0.1f;
                }
                else {
                    ShaderNodeOperation *node = (ShaderNodeOperation*)&shaderNodes[1];
                    
                    node->smoothness += 0.05f;
                }
                RebuildShader(&shader);
            }
        }
        
        if (activeTextboxIndex == 0) {
            if (shaderEditData.selectedIndex > -1) {
                if (IsKeyPressed(KEY_D)) {
                    shaderEditData.selectedIndex = DuplicateNode(shaderEditData.selectedIndex, true);
                    RebuildShader();
                }
                if (IsKeyPressed(KEY_DELETE)) {
                    DeleteNode(shaderEditData.selectedIndex);
                    shaderEditData.selectedIndex = -1;
                    RebuildShader();
                }
                if (IsKeyPressed(KEY_I)) {
                    ShowAddNodeDialog();
                }
            }
            
            // @TEMP
            if (IsKeyPressed(KEY_SPACE) && shaderEditData.selectedIndex > -1) {
                int childCount = shaderNodes[shaderEditData.selectedIndex].childNodeCount;
                TraceLog(LOG_INFO, "Selected node (#%d) has %d children", shaderEditData.selectedIndex, childCount);
                for (int i = -2; i < childCount+2; i++) {
                    int child = GetShaderNodeChild(shaderNodes, shaderNodeCount, shaderEditData.selectedIndex, i);
                    TraceLog(LOG_INFO, "> Child %d : #%d", i, child);
                }
            }
        }
        
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);
            
            BeginTextureMode(viewportRenderTex);
            
            ClearBackground(WHITE);
            
            // @TODO calculating this all the time (https://registry.khronos.org/OpenGL-Refpages/gl2.1/xhtml/glFrustum.xml)
            Matrix modelViewMatrix, modelProjMatrix;
            BeginMode3D(camera);
                modelViewMatrix = rlGetMatrixModelview();
                modelProjMatrix = rlGetMatrixProjection();
            EndMode3D();
            
            SetShaderValueMatrix(shader, modelViewMatrixLoc, modelViewMatrix);
            SetShaderValueMatrix(shader, modelProjMatrixLoc, modelProjMatrix);
            
            BeginShaderMode(shader);
            rlEnableDepthTest();            // Enable DEPTH_TEST for 3D
            
            DrawRectangle(0, 0, viewportRenderTex.texture.width, viewportRenderTex.texture.height, WHITE);
            //rlDrawRenderBatchActive();
            
            BeginMode3D(camera);
            
            // This is how we draw a specific part of the raymarched area
            //DrawCubeV((Vector3){ 0.0,0.0, 0.0}, Vector3Scale((Vector3){1.0f,1.0f,1.0f}, 2.0f), MAGENTA);
            
            EndShaderMode();
            
            //rlDisableDepthMask();
            
            float prevLineWidth = rlGetLineWidth();
            
            if (shaderEditData.selectedIndex > -1) {
                if (showWires > 0) {
                    EndMode3D();
                    BeginMode3D(camera);
                    rlSetLineWidth(5.0f);
                    DrawDebugNode(shaderEditData.selectedIndex, true, 1.0f, 0.001f);
                    // @TODO figure out why I need to do this!
                    // Tried with `rlDrawRenderBatchActive()`; doesn't work
                    EndMode3D();
                    BeginMode3D(camera);
                    if (showWires == 2) {
                        rlDisableDepthTest();
                        rlDisableDepthMask();
                        rlSetLineWidth(3.0f);
                        DrawDebugNode(shaderEditData.selectedIndex, true, 0.25f, 0.001f);
                        //rlSetLineWidth(prevLineWidth);
                        EndMode3D();
                        BeginMode3D(camera);
                        rlEnableDepthMask();
                    }
                }
            }
            
            rlDrawRenderBatchActive();
            rlSetLineWidth(prevLineWidth);
            rlEnableDepthTest();
            DrawGrid(10, 1.0f);
            
            EndMode3D();
            
            EndTextureMode();
            
            {
                // Flippin' OpenGL textures!
                Rectangle src = { 0.0f, 0.0f, editorWindowRects[WINDOW_VIEWPORT].width, -editorWindowRects[WINDOW_VIEWPORT].height };
                Rectangle dst = editorWindowRects[WINDOW_VIEWPORT];
                DrawTexturePro(viewportRenderTex.texture, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
            }
            // Gui stuff
            if (activeEditorWindow == WINDOW_SELECT_NODE_TYPE) {
                GuiLock();
            }
            
            HandleWindowHierarchy();
            HandleWindowProperties();
            
            if (activeEditorWindow == WINDOW_SELECT_NODE_TYPE) {
                GuiUnlock();
                HandleWindowSelectNodeType();
            }
            if (GuiIsLocked()) {
                TraceLog(LOG_WARNING, "Force-unlocking gui");
                GuiUnlock();
            }
            
            if (shaderWriterError != 0) {
                DrawText(shaderWriterErrorDesc, 220, GetScreenHeight() - 20, 10, RED);
            }

            //DrawText("(c) Raymarching shader by Iñigo Quilez. MIT License.", GetScreenWidth() - 280, GetScreenHeight() - 20, 10, BLACK);
            
            DrawFPS(GetScreenWidth() - 120, 20);
            DrawText(TextFormat("Debug lines: %s (TAB to change)", (showWires == 0 ? "NONE" : (showWires == 1 ? "Visible" : "All"))), editorWindowRects[WINDOW_HIERARCHY].width + 20, GetScreenHeight() - 50, 20, LIME);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadShader(shader);           // Unload shader
    FreeShaderWriter(&shaderWriterData);

    CloseWindow();                  // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

void RefreshShaderLocs() {
    viewEyeLoc    = GetShaderLocation(shader, "viewEye");
    viewCenterLoc = GetShaderLocation(shader, "viewCenter");
    runTimeLoc    = GetShaderLocation(shader, "runTime");
    resolutionLoc = GetShaderLocation(shader, "resolution");
    thingyLoc     = GetShaderLocation(shader, "thingy");
    cameraUpLoc   = GetShaderLocation(shader, "cameraUp");
    
    modelViewMatrixLoc = GetShaderLocation(shader, "modelViewMatrix");
    modelProjMatrixLoc = GetShaderLocation(shader, "modelProjMatrix");

    Vector2 resolution = { viewportRenderTex.texture.width, viewportRenderTex.texture.height };
    SetShaderValue(shader, resolutionLoc, &resolution, SHADER_UNIFORM_VEC2);
    
    float thingy = 1.0f / tan(camera.fovy * 0.5 * DEG2RAD);    
    SetShaderValue(shader, thingyLoc, &thingy, SHADER_UNIFORM_FLOAT);
}

// @TODO don't dynamically allocate all the time
bool RebuildShader() {
    int byteCount = WriteShader(&shaderWriterData, shaderNodes, shaderNodeCount, 0, 0);
    if (byteCount == 0) {
        TraceLog(LOG_ERROR, "Failed to write shader");
        return false;
    }
    else {
        byteCount++; // For the null terminator
        
        //TraceLog(LOG_INFO, "Shader takes up %d bytes", byteCount);
        char *buffer = (char*)MemAlloc(byteCount);
        byteCount = WriteShader(&shaderWriterData, shaderNodes, shaderNodeCount, buffer, byteCount);
        
        // temp
        SaveFileText("shader_temp.vs", buffer);
        
        Shader newShader = LoadShaderFromMemory(0, buffer);
        
        if (newShader.id == rlGetShaderIdDefault()) {
            TraceLog(LOG_ERROR, "Failed to load shader from memory, saved file for reference");
            SaveFileText("shader_temp_error.vs", buffer);
            MemFree(buffer);
            return false;
        }
        else {
            MemFree(buffer);
            UnloadShader(shader);
            shader = newShader;
            TraceLog(LOG_INFO, "Loaded shader from memory");
            RefreshShaderLocs();
            return true;
        }
        
    }
}

bool RecalcShaderNodesHierarchy() {
    // First clear all the child counts
    for (int i = 0; i < shaderNodeCount; i++) {
        shaderNodes[ i ].childNodeCount = 0;
    }
    
    #define DEBUG_STACK(prefix) { char buf[1024] = {0}; for (int j = 0; j < stackPos; j++) { sprintf(buf, "%s-> (%d) ", buf, stack[j]); } TraceLog(LOG_INFO, "[%s] Stack: %s", prefix, buf); }
    
    
    int stack[256] = { 0 };
    int stackPos = 0;
    
    // Reset in case something stupid has happened
    shaderEditData.nodes[0].depth = 0;
    int lastDepth = -1;
    for (int i = 1; i < shaderNodeCount; i++) {
        TraceLog(LOG_INFO, "Processing node #%d, lastDepth = %d", i, lastDepth);
        //DEBUG_STACK("before");
        int delta = shaderEditData.nodes[i].depth - lastDepth;
        if (delta > 0) {
            // One deeper (at least we hope so)
            if (delta > 1) {
                //TraceLog(LOG_WARNING, "Node %d depth increased by %d (%d to %d)", i, delta, lastDepth, shaderEditData.nodes[i].depth);
            }
            // And we're now the parent
            stackPos += delta;
            stack[stackPos] = i;
            // The parent has a new child
            if (stackPos > 1) {
                //TraceLog(LOG_INFO, "Processing node %d [delta = %d], parent = %d", i, delta, stack[stackPos-1]);
                shaderNodes[ stack[stackPos-1] ].childNodeCount++;
            }
        }
        else if (delta == 0) {
            // Same depth as before
            // The parent has a new child
            if (stackPos > 1) {
                //TraceLog(LOG_INFO, "Processing node %d [delta = %d], parent = %d", i, delta, stack[stackPos-1]);
                shaderNodes[ stack[stackPos-1] ].childNodeCount++;
            }
            // And adjust last pointer
            stack[stackPos] = i;
        }
        else {
            // Popping
            stackPos += delta;
            if (stackPos > 1) {
                //TraceLog(LOG_INFO, "Processing node %d [delta = %d], parent = %d", i, delta, stack[stackPos-1]);
                shaderNodes[ stack[stackPos-1] ].childNodeCount++;
            }
            // And adjust last pointer
            stack[stackPos] = i;
        }
        lastDepth = shaderEditData.nodes[i].depth;
        
        //DEBUG_STACK("after");
    }
    
    RecalcShaderNodesNav(shaderNodes, shaderNodesNav, shaderNodeCount);
    
    return true;
}

// @TODO keep this in sync
const char *GetShaderNodeTypeName(ShaderNodeType type) {
    switch(type) {
        case SHADER_NODE_NONE:            return "None";
        case SHADER_NODE_OBJECT:          return "Object";
        case SHADER_NODE_OP_UNION:        return "Union";
        case SHADER_NODE_OP_INTERSECTION: return "Intersection";
        case SHADER_NODE_OP_SUBTRACTION:  return "Subtraction";
        case SHADER_NODE_OP_SMOOTH:       return "Smooth";
        case SHADER_NODE_OP_TRANSLATION:  return "Translation";
        case SHADER_NODE_OP_ROTATION:     return "Rotation";
        case SHADER_NODE_OP_SCALE:        return "Scale";
        case SHADER_NODE_PLANE:           return "Plane";
        case SHADER_NODE_BOX:             return "Box";
        case SHADER_NODE_SPHERE:          return "Sphere";
        case SHADER_NODE_ELLIPSOID:       return "Ellipsoid";
        case SHADER_NODE_CAPSULE:         return "Capsule";
        case SHADER_NODE_CYLINDER:        return "Cylinder";
        default: return "Unknown";
    }
}

int GetShaderNodeTypeIcon(ShaderNodeType type) {
    switch(type) {
        case SHADER_NODE_NONE:            return ICON_HELP;
        case SHADER_NODE_OBJECT:          return ICON_PLAYER_JUMP;
        case SHADER_NODE_OP_UNION:        return ICON_UNION;
        case SHADER_NODE_OP_INTERSECTION: return ICON_INTERSECTION;
        case SHADER_NODE_OP_SUBTRACTION:  return ICON_DIFFERENCE;
        case SHADER_NODE_OP_SMOOTH:       return ICON_WAVE_SINUS;
        case SHADER_NODE_OP_TRANSLATION:  return ICON_TARGET_MOVE;
        case SHADER_NODE_OP_ROTATION:     return ICON_ROTATE;
        case SHADER_NODE_OP_SCALE:        return ICON_SCALE;
        //case SHADER_NODE_PLANE:           return ICON_NONE;
        case SHADER_NODE_BOX:             return ICON_CUBE;
        case SHADER_NODE_SPHERE:          return ICON_SPHERE;
        case SHADER_NODE_ELLIPSOID:       return ICON_ELLIPSOID;
        case SHADER_NODE_CAPSULE:         return ICON_CAPSULE;
        case SHADER_NODE_CYLINDER:        return ICON_CYLINDER;
        default: return ICON_ERROR;
    }
}

const char *GetShaderNodeName(int i) {
    switch(shaderNodes[i].type) {
        case SHADER_NODE_OP_UNION:
        case SHADER_NODE_OP_INTERSECTION:
        case SHADER_NODE_OP_SUBTRACTION:
            if ( ((ShaderNodeOperation*)(&shaderNodes[i]))->smoothness > 0.0f ) {
                return TextFormat("Smooth %s", GetShaderNodeTypeName(shaderNodes[i].type));
            }
        default:
    }
    return GetShaderNodeTypeName(shaderNodes[i].type);
}

void ApplyNodeTransform(int i) {
    if (i == -1) return;
    
    int parent = shaderNodesNav[i].parent;
    
    if (parent != -1) {
        ApplyNodeTransform(parent);
    }
    
    ShaderNodeOperation *node = (ShaderNodeOperation*)&shaderNodes[i];
    
    switch(node->type) {
        case SHADER_NODE_OP_TRANSLATION:
            rlTranslatef(node->translation.x, node->translation.y, node->translation.z);
            break;
        case SHADER_NODE_OP_ROTATION:
            rlRotatef(node->rotation.w * RAD2DEG, node->rotation.x, node->rotation.y, node->rotation.z);
            break;
        default:
    }
}

void DrawDebugNode(int i, bool primary, float alpha, float offset) {
    // Transforms
    
    rlPushMatrix();
    ApplyNodeTransform(shaderNodesNav[i].parent);
    
    ShaderNodePrimitive* node = (ShaderNodePrimitive*)&shaderNodes[i];
    
    const float minBrightness = 0.0f;
    const float maxBrightness = 0.25f;
    const float phase = sinf(2.0f * (float)GetTime() * PI)*0.5f+0.5f;
    
    Color colour = Fade(ColorBrightness(primary ? BLUE : BLUE, minBrightness + (maxBrightness-minBrightness)*phase), alpha);
    //Color colour = Fade(ColorBrightness(primary ? MAGENTA : BLUE, -1.0f), alpha);
    
    //colour = Fade(RED, 0.1f);
    
    switch(shaderNodes[i].type) {
        case SHADER_NODE_OBJECT:
        case SHADER_NODE_OP_UNION:
        case SHADER_NODE_OP_INTERSECTION:
            goto draw_child_nodes; // Yayyyy goto
            break;
        case SHADER_NODE_OP_SMOOTH:
            offset += ((ShaderNodeOperation*)node)->smoothness;
            goto draw_child_nodes; // Yayyyy goto
            break;
        case SHADER_NODE_OP_TRANSLATION:
        case SHADER_NODE_OP_ROTATION:
        case SHADER_NODE_OP_SCALE:
draw_child_nodes:
            rlPopMatrix();
            if (shaderNodes[i].childNodeCount > 0) {
                for (int j = 0; j < shaderNodes[i].childNodeCount; j++) {
                    DrawDebugNode(i+j+1, false, alpha, offset);
                }
            }
            rlPushMatrix();
            ApplyNodeTransform(shaderNodesNav[i].parent);
            break;
        
        case SHADER_NODE_PLANE:
            break;
        case SHADER_NODE_SPHERE:
            // @NOTE spheres need to be offset more because their lines would be *inside* the actual sphere
            DrawSphereWires2(node->centre, node->size.x + offset + 0.05, 20, 20, colour);
            //DrawSphereWires(node->centre, node->size.x * drawScaleFactor, 20, 20, Fade(RED, 0.1f));
            break;
        case SHADER_NODE_BOX:
            DrawCubeWiresV(node->centre, Vector3Scale(Vector3AddValue(node->size, offset), 2.0f), colour);
            break;
        case SHADER_NODE_ELLIPSOID:
            rlTranslatef(node->centre.x, node->centre.y, node->centre.z);
            rlScalef(node->size.x, node->size.y, node->size.z);
            DrawSphereWires2((Vector3){0,0,0}, 1.0f + offset + 0.05, 20, 20, colour);
            //DrawCubeWiresV(node->centre, Vector3Scale(Vector3AddValue(node->size, offset), 2.0f), colour);
            rlScalef(1.0f/node->size.x, 1.0f/node->size.y, 1.0f/node->size.z);
            rlTranslatef(-node->centre.x, -node->centre.y, -node->centre.z);
            break;
        case SHADER_NODE_CYLINDER:
            DrawCylinderWiresEx(node->p1, node->p2, node->radius, node->radius, 10, colour);
            break;
        case SHADER_NODE_CAPSULE:
            DrawCapsuleWires(node->p1, node->p2, node->radius, 10, 10, colour);
            break;
        case SHADER_NODE_NONE:
        default:
            break;
    }
    
    rlPopMatrix();
}

int GuiSpinnerFloat(Rectangle bounds, const char *text, float *value, float minValue, float maxValue, int precision, bool editMode) {
    int intValue    = floorf(*value   * 100.0f);
    int intMinValue = floorf(minValue * 100.0f);
    int intMaxValue = floorf(maxValue * 100.0f);
    
    int ret = GuiSpinner(bounds, text, &intValue, intMinValue, intMaxValue, editMode);
    
    *value = (float)intValue * 0.01f;
    
    return ret;
}

// Draw sphere wires
void DrawSphereWires2(Vector3 centerPos, float radius, int rings, int slices, Color color)
{
    rlPushMatrix();
        // NOTE: Transformation is applied in inverse order (scale -> translate)
        rlTranslatef(centerPos.x, centerPos.y, centerPos.z);
        rlScalef(radius, radius, radius);

        rlBegin(RL_LINES);
            rlColor4ub(color.r, color.g, color.b, color.a);

            for (int i = 0; i < (rings + 2); i++)
            {
                for (int j = 0; j < slices; j++)
                {
                    /*
                    rlVertex3f(cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*i))*sinf(DEG2RAD*(360.0f*j/slices)),
                               sinf(DEG2RAD*(270 + (180.0f/(rings + 1))*i)),
                               cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*i))*cosf(DEG2RAD*(360.0f*j/slices)));
                    rlVertex3f(cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1)))*sinf(DEG2RAD*(360.0f*(j + 1)/slices)),
                               sinf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1))),
                               cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1)))*cosf(DEG2RAD*(360.0f*(j + 1)/slices)));
                               */

                    rlVertex3f(cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1)))*sinf(DEG2RAD*(360.0f*(j + 1)/slices)),
                               sinf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1))),
                               cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1)))*cosf(DEG2RAD*(360.0f*(j + 1)/slices)));
                    rlVertex3f(cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1)))*sinf(DEG2RAD*(360.0f*j/slices)),
                               sinf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1))),
                               cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1)))*cosf(DEG2RAD*(360.0f*j/slices)));

                    rlVertex3f(cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1)))*sinf(DEG2RAD*(360.0f*j/slices)),
                               sinf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1))),
                               cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1)))*cosf(DEG2RAD*(360.0f*j/slices)));
                    rlVertex3f(cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*i))*sinf(DEG2RAD*(360.0f*j/slices)),
                               sinf(DEG2RAD*(270 + (180.0f/(rings + 1))*i)),
                               cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*i))*cosf(DEG2RAD*(360.0f*j/slices)));
                }
            }
        rlEnd();
    rlPopMatrix();
}

void RecursiveSetNodeExpanded(int index, bool expanded) {
    shaderEditData.nodes[index].expanded = expanded;
    int depth = shaderEditData.nodes[index].depth;
    for (int i = index+1; i < shaderNodeCount; i++) {
        if (shaderEditData.nodes[i].depth <= depth) {
            return;
        }
        shaderEditData.nodes[i].expanded = expanded;
    }
}

void ExpandToShowNode(int index) {
    /*
    // Bounds check (@TODO warning?)
    if (index < 0 || index >= shaderNodeCount) return;
    
    // Nothing to do if this is the root node
    if (index == 0) return;
    
    do {
        index = GetShaderNodeParent(shaderNodes, shaderNodeCount, index);
        if (index == -1) return;
        shaderEditData.nodes[index].expanded = true;
    } while (index != -1);
    */
}

int CalcNodeDescendantCount(int nodeIndex) {
    int depth = shaderEditData.nodes[nodeIndex].depth;
    int descendantCount = 0;
    for (int i = nodeIndex+1; i < shaderNodeCount; i++) {
        if (shaderEditData.nodes[i].depth <= depth) {
            break;
        }
        descendantCount++;
    }
    return descendantCount;
}

bool IsNodeDescendant(int child, int parent) {
    
    int delta = child - parent;
    return delta > 0 && delta <= shaderNodesNav[parent].descendantCount;
            //TraceLog(LOG_ERROR, "Node #%d descendant of node %d? %d [count = %d, delta = %d]", i, j, IsNodeDescendant(i, j), shaderNodesNav[j].descendantCount, i-j);
    
    //if (child <= parent) return false;
    //return (parent-child < CalcNodeDescendantCount(parent));
}

int DuplicateNode(int nodeIndex, bool duplicateChildren) {
    if (nodeIndex < 0 || nodeIndex >= shaderNodeCount) {
        // Invalid node index
        return -1;
    }
    
    int insertCount = 1;
    if (duplicateChildren) {
        insertCount += CalcNodeDescendantCount(nodeIndex);
    }
    
    if (shaderNodeCount+insertCount > SHADER_NODES_MAX) {
        // Not enough space in buffer
        return -1;
    }
    
    int newIndex = nodeIndex + insertCount;
    
    TraceLog(LOG_INFO, "Need to insert %d nodes", insertCount);
    
    // Move (well, copy) nodes out of the way
    for (int i = shaderNodeCount-1 + insertCount; i >= newIndex + insertCount; i--) {
        int dstSlot = i;
        int srcSlot = dstSlot - insertCount;
        shaderNodes[dstSlot] = shaderNodes[srcSlot];
        shaderEditData.nodes[dstSlot] = shaderEditData.nodes[srcSlot];
        TraceLog(LOG_INFO, "[move] Copied node #%d to node %d", srcSlot, dstSlot);
    }
    // Insert the new ones
    for (int i = 0; i < insertCount; i++) {
        shaderNodes[newIndex+i] = shaderNodes[nodeIndex+i];
        shaderEditData.nodes[newIndex+i] = shaderEditData.nodes[nodeIndex+i];
        TraceLog(LOG_INFO, "[insert] Copied node #%d to node %d", nodeIndex+i, newIndex+i);
    }
    // Update the total
    shaderNodeCount += insertCount;
    
    // Recalc (mostly just for child count)
    RecalcShaderNodesHierarchy();
    
    // Return new index
    return newIndex;
}

bool DeleteNode(int nodeIndex) {
    int descendantCount = CalcNodeDescendantCount(nodeIndex);
    int deleteCount = descendantCount+1;
    
    for (int i = nodeIndex; i < shaderNodeCount-deleteCount; i++) {
        shaderNodes[i] = shaderNodes[i+deleteCount];
        shaderEditData.nodes[i] = shaderEditData.nodes[i+deleteCount];
    }
    
    shaderNodeCount = shaderNodeCount-deleteCount;
    
    RecalcShaderNodesHierarchy();
    
    return true;
}

bool CanDragNode(int source, int target, DragTargetMode mode) {
    if (source < 0 || source >= shaderNodeCount) return false;
    if (target < 0 || target >= shaderNodeCount) return false;
    
    bool debug = false;
    if ((mode & DRAG_FLAG_DEBUG) == DRAG_FLAG_DEBUG) {
        TraceLog(LOG_WARNING, "before : %d, after : %d", mode, mode & ~DRAG_FLAG_DEBUG);
        debug = true;
        mode &= ~DRAG_FLAG_DEBUG;
    }
    
    // Cannot drag a node onto any of its descendants
    // (@TODO exception : if the descendant is literally the last item and we want to go below)
    if (IsNodeDescendant(target, source)) {
        if (debug) {
            TraceLog(LOG_WARNING, "Node #%d is a descendant of node #%d", target, source);
        }
        return false;
    }
    // Cannot drop a node onto itself either
    if (source == target) {
        if (debug) {
            TraceLog(LOG_WARNING, "Node #%d is ... node #%d. Wow.", target, source);
        }
        return false;
    }
    
    // This seems to make sense (?)
    if (mode == DRAG_AFTER) {
        if (shaderNodes[target].childNodeCount > 0 && shaderEditData.nodes[target].expanded){
            if (debug) {
                TraceLog(LOG_WARNING, "Can't remember the logic here but... Node #%d and #%d => nope.", target, source);
            }
            return false;
        }
    }
    
    // I guess??
    if (debug) {
        TraceLog(LOG_WARNING, "dragging %d onto %d is ok", source, target);
    }
    return true;
}

void DoDropNode(int source, int target, DragTargetMode mode) {
    static ShaderNode *scratchNodes = 0;
    static ShaderEditorNodeData *scratchEditorNodes = 0;
    static int scratchNodeCount = 0;
    
    if (!CanDragNode(source, target, mode)) {
        TraceLog(LOG_WARNING, "Attepting to drop node #%d onto node #%d, mode = %d : not allowed", source, target, mode);
        return;
    }
    
    if (mode != DRAG_ONTO) {
        TraceLog(LOG_WARNING, "Only drop-onto is implemented right now");
        return;
    }
    
    // @TODO this needs to be more efficient
    if (shaderNodeCount > scratchNodeCount) {
        if (scratchNodes != 0) MemFree(scratchNodes);
        if (scratchEditorNodes != 0) MemFree(scratchEditorNodes);
        
        scratchNodeCount = shaderNodeCount + 10;
        if (scratchNodeCount < 100) scratchNodeCount = 100;
        
        scratchNodes = (ShaderNode*)MemAlloc(scratchNodeCount * sizeof(ShaderNode));
        scratchEditorNodes = (ShaderEditorNodeData*)MemAlloc(scratchNodeCount * sizeof(ShaderEditorNodeData));
    }
    
    // Ugh.... just copy the whole thing. 
    memcpy(scratchNodes, shaderNodes, shaderNodeCount * sizeof(ShaderNode));
    memcpy(scratchEditorNodes, shaderEditData.nodes, shaderNodeCount * sizeof(ShaderEditorNodeData));
    
    #define COPY_FROM_SCRATCH(from, to, length) {\
        TraceLog(LOG_INFO, "Copying [%d-%d] (count %d) to [%d-%d]", from, from+length-1, length, to, to+length-1);\
        memcpy(&shaderNodes[to], &scratchNodes[from], (length)*sizeof(ShaderNode));\
        memcpy(&shaderEditData.nodes[to], &scratchEditorNodes[from], (length)*sizeof(ShaderEditorNodeData));\
    }
    
    // Adjust target to reflect mode
    int depthDelta = 0;
    
    // DRAG_ONTO
    if (mode == DRAG_ONTO) {
        depthDelta = 1 + shaderEditData.nodes[target].depth - shaderEditData.nodes[source].depth;
        target = target + CalcNodeDescendantCount(target) + 1;
    }
    
    int sourceSize = 1+CalcNodeDescendantCount(source);
    //COPY_FROM_SCRATCH(source, target, sourceSize);
    
    if (target > source) {
        COPY_FROM_SCRATCH(source+sourceSize, source, target - source);
        COPY_FROM_SCRATCH(source, target-sourceSize, sourceSize);
        for (int i = 0; i < sourceSize; i++) {
            shaderEditData.nodes[target + i - sourceSize].depth += depthDelta;
        }
    }
    else {
        COPY_FROM_SCRATCH(source, target, sourceSize);
        COPY_FROM_SCRATCH(target, target+sourceSize, source - target);
        for (int i = 0; i < sourceSize; i++) {
            shaderEditData.nodes[target + i].depth += depthDelta;
        }
    }
    /*
    for (int i = 0; i < sourceSize; i++) {
        shaderEditData.nodes[target + i].depth += depthDelta;
    }
    */
    
    RecalcShaderNodesHierarchy();
    TraceLog(LOG_INFO, "Rebuilding shader...");
    RebuildShader();
    TraceLog(LOG_INFO, "Done rebuilding shader");
}

int WindowAtPoint(Vector2 point) {
    for (int i = 1; i < WINDOW_MAX; i++) {
        if (CheckCollisionPointRec(point, editorWindowRects[i])) {
            return i;
        }
    }
    return WINDOW_VIEWPORT;
}

void RecalcWindowLayout() {
    // @TODO resizable
    float eastSize = 300;
    
    float margin = 5;
    
    //editorWindowRects[WINDOW_HIERARCHY] = (Rectangle){ 5, 5, 200, (GetScreenHeight() - 10)/2 };
    //editorWindowRects[WINDOW_PROPERTIES] = (Rectangle){ 5, ((float)GetScreenHeight()+5)/2, 200 , (GetScreenHeight() - 10)/2 };
    
    editorWindowRects[WINDOW_HIERARCHY]  = (Rectangle){ margin, margin, eastSize, (GetScreenHeight() - margin*2)/2 };
    editorWindowRects[WINDOW_PROPERTIES] = (Rectangle){ margin, ((float)GetScreenHeight()+margin)/2, eastSize , (GetScreenHeight() - margin*2)/2 };
    
    editorWindowRects[WINDOW_VIEWPORT]   = (Rectangle){ eastSize + margin, 0, GetScreenWidth() - eastSize - margin, GetScreenHeight() };
    
    if (viewportRenderTex.id > 0) {
        TraceLog(LOG_INFO, "Unloading render texture");
        UnloadRenderTexture(viewportRenderTex);
    }
    
    viewportRenderTex = LoadRenderTexture(editorWindowRects[WINDOW_VIEWPORT].width, editorWindowRects[WINDOW_VIEWPORT].height);
    TraceLog(LOG_INFO, "Created render texture %dx%d", viewportRenderTex.texture.width, viewportRenderTex.texture.height);
    
    //Vector2 resolution = { (float)GetScreenWidth(), (float)GetScreenHeight() };
    Vector2 resolution = { editorWindowRects[WINDOW_VIEWPORT].width, editorWindowRects[WINDOW_VIEWPORT].height };
    SetShaderValue(shader, resolutionLoc, &resolution, SHADER_UNIFORM_VEC2);
}

void HandleWindowHierarchy() {
    Rectangle windowRect = editorWindowRects[WINDOW_HIERARCHY];
    
    GuiPanel(windowRect, "Hierarchy");
    
    Rectangle clientRect = (Rectangle){ windowRect.x + 8, windowRect.y + 30, windowRect.width - 16, windowRect.height - 40 };
    
    bool windowHot = (!GuiIsLocked()) && CheckCollisionPointRec(GetMousePosition(), windowRect);
    bool clicked = windowHot && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool selectNothing = clicked;
    
    Rectangle itemRect = clientRect; itemRect.height = 20;
    
    int dragTargetIndex = -1;
    DragTargetMode dragTargetMode = DRAG_NONE;
    float dragTargetItemY = 0;
    float dragTargetItemX = 0; // Accounts for indent
    

    int prevAlign = GuiGetStyle(TOGGLE, TEXT_ALIGNMENT);
    GuiSetStyle(TOGGLE, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    
    int skipUntilDepth = -1;
    int prevRenderedNode = -1;
    
    int toggleExpansionNode = -1;
    bool toggleExpansionRecursive = false;
    
    bool showAddNodeDialog = false;
    
    for (int i = 0; i < shaderNodeCount; i++) {
        
        if (skipUntilDepth != -1) {
            if (shaderEditData.nodes[i].depth <= skipUntilDepth) {
                skipUntilDepth = -1;
            }
            else {
                continue;
            }
        }
        if (!shaderEditData.nodes[i].expanded) {
            skipUntilDepth = shaderEditData.nodes[i].depth;
        }
        
        int indent = (2+shaderEditData.nodes[i].depth);
        
        bool active = shaderEditData.selectedIndex == i;
        bool hot = windowHot && CheckCollisionPointRec(GetMousePosition(), itemRect);
        
        int x = itemRect.x + GuiGetStyle(TOGGLE, TEXT_PADDING) + GuiGetStyle(TOGGLE, BORDER_WIDTH);
        int y = itemRect.y + itemRect.height / 2;
        
        bool hasExpando = shaderNodes[i].childNodeCount > 0;
        Rectangle expandoRect = (Rectangle){ itemRect.x + (indent-2) * 16, y - 16/2, 16, 16 };
        
        bool expandoHot = hasExpando && CheckCollisionPointRec(GetMousePosition(), expandoRect);
        
        bool hasAddButton = hot;
        Rectangle addButtonRect;
        bool addButtonHot = false;
        
        // ---- moved block -----
        
        int state = STATE_NORMAL;
        if (hot) state = STATE_FOCUSED;
        if (active || clicked) state = STATE_PRESSED;
        if (GuiIsLocked() || GuiGetState() == STATE_DISABLED) state = STATE_DISABLED;
        
        if (shaderEditData.draggingIndex != -1) {
            
            if (hot) {
                if (shaderEditData.draggingIndex == i) {
                    // Dragging onto myself? No thanks
                    state = STATE_DISABLED;
                }
                else {
                    dragTargetIndex = i;
                    dragTargetItemY = itemRect.y;
                    dragTargetItemX = itemRect.x + indent * 16 - 16;
                    float yy = (GetMousePosition().y - itemRect.y)/itemRect.height;
                    if (yy < .35) {
                        dragTargetMode = DRAG_BEFORE;
                        state = STATE_NORMAL;
                    }
                    else if (yy > .65 && shaderNodes[i].childNodeCount == 0) {
                        dragTargetMode = DRAG_AFTER;
                        state = STATE_NORMAL;
                    }
                    else {
                        dragTargetMode = DRAG_ONTO;
                        state = STATE_FOCUSED;
                    }
                }
            }
            else {
                if (shaderEditData.draggingIndex == i) {
                    state = STATE_PRESSED;
                }
                else {
                    state = STATE_NORMAL;
                }
            }
        }
        
        if (shaderEditData.draggingIndex == -1 || shaderEditData.draggingIndex == i || (dragTargetIndex == i && dragTargetMode == DRAG_ONTO)) {
            if (active || hot) {
                bool wasLocked = GuiIsLocked();
                if (hot && shaderEditData.draggingIndex == i) GuiLock();
                GuiToggle(itemRect, NULL, &active);
                if (hot && shaderEditData.draggingIndex == i && !wasLocked) GuiUnlock();
                    
                if (active && !expandoHot && shaderEditData.draggingIndex == -1) {
                    shaderEditData.selectedIndex = i;
                }
                // If *this* item is hot and the mouse is pressed, do not deselect
                if (hot && clicked) selectNothing = false;
                
                if (shaderEditData.draggingIndex == -1) {
                    if (hot && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && Vector2Length(GetMouseDelta()) > 0.0f) {
                        shaderEditData.draggingIndex = i;
                        shaderEditData.draggingOffset = Vector2Subtract(GetMousePosition(), (Vector2){itemRect.x + (indent-1) * 16, itemRect.y});
                        TraceLog(LOG_INFO, "Dragging offset = %.1f, %.1f", shaderEditData.draggingOffset.x, shaderEditData.draggingOffset.y);
                        TraceLog(LOG_INFO, "> Item Rect = %.1f, %.1f", itemRect.x, itemRect.y);
                        TraceLog(LOG_INFO, "> Mouse Pos = %.1f, %.1f", GetMousePosition().x, GetMousePosition().y);
                    }
                }
            }
            if (clicked && expandoHot) {
                toggleExpansionNode = i;
                toggleExpansionRecursive = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
            }
        }
        
        // This is where the block was
        
        
        
        int iconId = GetShaderNodeTypeIcon(shaderNodes[i].type);
        
        Color colour = GetColor(GuiGetStyle(TOGGLE, TEXT + state*3));
        
        GuiDrawIcon(iconId, itemRect.x + indent * 16 - 16, itemRect.y + itemRect.height/2 - 16/2, 1, colour);
        char *name = GetShaderNodeName(i);
        DrawText(name, x + indent * 16 + 2, itemRect.y + itemRect.height/2 - 10/2, 10, colour);
        
        if (hasAddButton) {
            addButtonRect = (Rectangle){ x + indent * 16 + 2 + MeasureText(name, 10) + 4, itemRect.y + itemRect.height/2 - 16/2, 16, 16 };
            addButtonHot = CheckCollisionPointRec(GetMousePosition(), addButtonRect);
            int addButtonIcon = addButtonHot ? ICON_ADD_FILL : ICON_ADD;
            Color addButtonColour = (state != STATE_DISABLED && addButtonHot) ? GetColor(GuiGetStyle(TOGGLE, TEXT + STATE_PRESSED*3)) : colour;
            GuiDrawIcon(addButtonIcon, addButtonRect.x, addButtonRect.y, 1, addButtonColour);
        }
        
        if (hasExpando) {
            int expandoIcon = (state != STATE_DISABLED && expandoHot) ? (shaderEditData.nodes[i].expanded ? ICON_TREE_COLLAPSE_FILL : ICON_TREE_EXPAND_FILL) :(shaderEditData.nodes[i].expanded ? ICON_TREE_COLLAPSE : ICON_TREE_EXPAND);
            //GuiDrawIcon(expandoIcon, expandoRect.x, expandoRect.y, 1, (state != STATE_DISABLED && expandoHot) ? GetColor(GuiGetStyle(TOGGLE, TEXT + STATE_PRESSED*3)) : GetColor(GuiGetStyle(TOGGLE, TEXT + STATE_NORMAL*3)));
            Color expandoColour = (state != STATE_DISABLED && expandoHot) ? GetColor(GuiGetStyle(TOGGLE, TEXT + STATE_PRESSED*3)) : colour;
            GuiDrawIcon(expandoIcon, expandoRect.x, expandoRect.y, 1, expandoColour);
        }
        
        if (shaderWriterError != 0 && i >= shaderWriterErrorNode) {
            GuiDrawIcon(ICON_ERROR, itemRect.x + itemRect.width - 4 - 16, itemRect.y + itemRect.height/2 - 16/2, 1, RED);
        }
        
        // Draw ID for reference (debug)
        DrawText(TextFormat("%d", i), itemRect.x + itemRect.width - 4 - 16, y - 10/2, 10, MAGENTA);
        
        //DrawText(TextFormat("%d", active), itemRect.x + itemRect.width - 4 - 16, y - 10/2, 10, MAGENTA);
        //DrawText(TextFormat("%d", state), itemRect.x + itemRect.width - 4 - 16, y - 10/2, 10, MAGENTA);
        //DrawText(TextFormat("%d", shaderNodes[i].childNodeCount), itemRect.x + itemRect.width - 4 - 16, y - 10/2, 10, MAGENTA);
        //DrawLine(itemRect.x, itemRect.y + itemRect.height/2, itemRect.x + indent * spaceWidth, itemRect.y + itemRect.height/2, MAGENTA);
        //DrawLine(x, itemRect.y + itemRect.height/2, x + indent * spaceWidth, itemRect.y + itemRect.height/2, MAGENTA);
        
        
        if (clicked && addButtonHot) {
            shaderEditData.selectedIndex = i;
            showAddNodeDialog = true;
        }
        
        prevRenderedNode = i;
        itemRect.y += itemRect.height;
    }
    
    if (shaderEditData.draggingIndex != -1) {
        
        // Draw drop target lines
        {
            if (dragTargetMode != DRAG_ONTO) {
                float y = dragTargetItemY;
                if (dragTargetMode == DRAG_AFTER) y += itemRect.height;
                
                Rectangle lineRect = itemRect;
                
                lineRect.width -= (dragTargetItemX - lineRect.x);
                lineRect.x = dragTargetItemX;
                lineRect.y = y - 1;
                lineRect.height = 2;
                
                DrawRectangleRec(lineRect, GetColor(GuiGetStyle(TOGGLE, BORDER + STATE_PRESSED*3)));
            }
        }
        
        Vector2 mousePos = GetMousePosition();
        
        const char *draggingNodeName = GetShaderNodeName(shaderEditData.draggingIndex);
        int iconId = GetShaderNodeTypeIcon(shaderNodes[shaderEditData.draggingIndex].type);
        
        Rectangle draggingItemRect = itemRect;
        int margin = (GuiGetStyle(TOGGLE, TEXT_PADDING) + GuiGetStyle(TOGGLE, BORDER_WIDTH));
        draggingItemRect.width = MeasureText(draggingNodeName, 10) + 16 + margin * 2 + 8;
        draggingItemRect.x = mousePos.x - shaderEditData.draggingOffset.x;
        draggingItemRect.y = mousePos.y - shaderEditData.draggingOffset.y;
        
        DrawRectangleRec(draggingItemRect, Fade(GetColor(GuiGetStyle(TOGGLE, BASE + STATE_PRESSED*3)), 0.75));
        DrawRectangleLinesEx(draggingItemRect, 1, GetColor(GuiGetStyle(TOGGLE, BORDER + STATE_PRESSED*3)));
        GuiDrawIcon(iconId, draggingItemRect.x + margin, draggingItemRect.y + draggingItemRect.height/2 - 16/2, 1, GetColor(GuiGetStyle(TOGGLE, TEXT + STATE_PRESSED*3)));
        DrawText(draggingNodeName, draggingItemRect.x + margin + 16, draggingItemRect.y + draggingItemRect.height/2 - 10/2, 10, GetColor(GuiGetStyle(TOGGLE, TEXT + STATE_PRESSED*3)));
        
        if (CanDragNode(shaderEditData.draggingIndex, dragTargetIndex, dragTargetMode)) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        }
        else {
            SetMouseCursor(MOUSE_CURSOR_NOT_ALLOWED);
        }
        
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            // Cancel drag
            shaderEditData.draggingIndex = -1;
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        }
        else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            
            TraceLog(LOG_INFO, "Want to drop node %d onto node %d, allowed = %d", shaderEditData.draggingIndex, dragTargetIndex, CanDragNode(shaderEditData.draggingIndex, dragTargetIndex, dragTargetMode | DRAG_FLAG_DEBUG));
            // DoDropNode
            if (CanDragNode(shaderEditData.draggingIndex, dragTargetIndex, dragTargetMode | DRAG_FLAG_DEBUG)) {
                TraceLog(LOG_INFO, "Beginning drop");
                DoDropNode(shaderEditData.draggingIndex, dragTargetIndex, dragTargetMode);
                TraceLog(LOG_INFO, "Finished drop");
            }
            
            shaderEditData.draggingIndex = -1;
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        }
    }
    
    GuiSetStyle(TOGGLE, TEXT_ALIGNMENT, prevAlign);
    
    if (selectNothing) {
        if (activeEditorWindow != WINDOW_SELECT_NODE_TYPE) {
            TraceLog(LOG_INFO, "unselecting node; hierarchy bg click");
            shaderEditData.selectedIndex = -1;
        }
    }
    
    if (toggleExpansionNode != -1) {
        //TraceLog(LOG_INFO, "Toggled expansion of node #%d to %d", toggleExpansionNode, !shaderEditData.nodes[toggleExpansionNode].expanded);
        shaderEditData.nodes[toggleExpansionNode].expanded = !shaderEditData.nodes[toggleExpansionNode].expanded;
        if (toggleExpansionRecursive) {
            RecursiveSetNodeExpanded(toggleExpansionNode, shaderEditData.nodes[toggleExpansionNode].expanded);
        }
    }
    
    if (showAddNodeDialog) {
        TraceLog(LOG_INFO, "Calling ShowAddNodeDialog");
        ShowAddNodeDialog();
    }
}

void HandleWindowProperties() {
    
    if (shaderEditData.selectedIndex == -1) {
        return HandleWindowPropertiesGeneral();
    }
    
    switch(shaderNodeTypeCategory[shaderNodes[shaderEditData.selectedIndex].type]) {
        case SHADER_NODE_TYPE_OBJECT:
            return HandleWindowPropertiesObject();
        case SHADER_NODE_TYPE_OPERATION:
            return HandleWindowPropertiesOperation();
        case SHADER_NODE_TYPE_PRIMITIVE:
            return HandleWindowPropertiesPrimitive();
        default:
    }
    
    // Anything selected we don't have properties for
    {
        Rectangle windowRect = editorWindowRects[WINDOW_PROPERTIES];
        Rectangle clientRect = (Rectangle){ windowRect.x + 8, windowRect.y + 30, windowRect.width - 16, windowRect.height - 40 };
        const char *title = TextFormat("Properties - %s", GetShaderNodeName(shaderEditData.selectedIndex));
        
        GuiPanel(windowRect, title);
        
        Rectangle itemRect = clientRect; itemRect.height = 20;
        GuiLabel(itemRect, "No properties");
    }
}

void HandleWindowPropertiesGeneral() {
    
    Rectangle windowRect = editorWindowRects[WINDOW_PROPERTIES];
    Rectangle clientRect = (Rectangle){ windowRect.x + 8, windowRect.y + 30, windowRect.width - 16, windowRect.height - 40 };
    const char *title = "Properties";
    
    GuiPanel(windowRect, title);
    
    Rectangle itemRect = clientRect; itemRect.height = 20;
    GuiLabel(itemRect, "No properties");
}

void HandleWindowPropertiesObject() {
    
    Rectangle windowRect = editorWindowRects[WINDOW_PROPERTIES];
    Rectangle clientRect = (Rectangle){ windowRect.x + 8, windowRect.y + 30, windowRect.width - 16, windowRect.height - 40 };
    const char *title = TextFormat("Properties - %s", GetShaderNodeName(shaderEditData.selectedIndex));
    
    GuiPanel(windowRect, title);
    
    Rectangle itemRect = clientRect; itemRect.height = 20;
    GuiLabel(itemRect, "Diffuse Colour");
    
    itemRect.y += itemRect.height + 5;
    itemRect.height = 100;
    
    ShaderNodeObject *node = (ShaderNodeObject*)&shaderNodes[shaderEditData.selectedIndex];
    
    itemRect.width -= 30;
    Color tmpColour = node->colDiffuse;
    if (GuiColorPicker(itemRect, 0, &tmpColour));
    if (ColorToInt(tmpColour) != ColorToInt(node->colDiffuse)) {
        node->colDiffuse = tmpColour;
        RebuildShader();
    }
}

void HandleWindowPropertiesOperation() {
    
    int windowId = WINDOW_PROPERTIES;
    
    Rectangle windowRect = editorWindowRects[WINDOW_PROPERTIES];
    Rectangle clientRect = (Rectangle){ windowRect.x + 8, windowRect.y + 30, windowRect.width - 16, windowRect.height - 40 };
    const char *title = TextFormat("Properties - %s", GetShaderNodeName(shaderEditData.selectedIndex));
    
    GuiPanel(windowRect, title);
    
    Rectangle itemRect = clientRect; itemRect.height = 20;
    
    ShaderNodeOperation* node = (ShaderNodeOperation*)&shaderNodes[shaderEditData.selectedIndex];
    
    // --------- Node type --------
    
    /*
    static bool debug = true;
    
    int oldTypeIndex = -1;
    int optionCount = 0;
    char options[4096] = { 0 };
    for (int i = 0; i < SHADER_NODE_MAX; i++) {
        if (shaderNodeTypeCategory[i] == SHADER_NODE_TYPE_OPERATION) {
            if  (options[0] != 0) strcat(options, ";");
            strcat(options, GetShaderNodeTypeName(i, true));
            
            if (i == shaderNodes[shaderEditData.selectedIndex].type) {
                oldTypeIndex = optionCount;
            }
            optionCount++;
        }
    }
    int newTypeIndex = oldTypeIndex;
    if (debug) {
        TraceLog(LOG_INFO, "Got %d options, selected = %d : %s", optionCount, oldTypeIndex, options);
        debug = false;
    }
    
    GuiComboBox(itemRect, options, &newTypeIndex);
    
    static bool debugChoose = true;
    if (newTypeIndex != oldTypeIndex) {
        int optionIndex = 0;
        for (int i = 0; i < SHADER_NODE_MAX; i++) {
            if (shaderNodeTypeCategory[i] == SHADER_NODE_TYPE_OPERATION) {
                if (newTypeIndex == optionIndex) {
                    if (debugChoose) {
                        TraceLog(LOG_INFO, "Chose option %d, which is node type %d [%s]", optionIndex, i, GetShaderNodeTypeName(i, false));
                        debugChoose = false;
                    }
                    shaderNodes[shaderEditData.selectedIndex].type = i;
                    RebuildShader();
                    break;
                }
                optionIndex++;
            }
        }
    }
    
    
    itemRect.y += itemRect.height;
    */
    
    
    float labelWidth = itemRect.width * 0.5f;
    
    bool somethingChanged = false;
    
    Rectangle labelRect = itemRect; labelRect.width = labelWidth;
    Rectangle valueRect = itemRect; valueRect.x += labelRect.width-1; valueRect.width -= labelRect.width-1;
    float deltaY = labelRect.height + 1;
    
    int textBoxId = 0;
    float indent = 16;
    
    #define ADD_FLOAT(label, ref) { if (HandleTextboxFloat(windowId, ++textBoxId, labelRect, label, valueRect, ref, indent)) {\
        somethingChanged |= !(activeEditorWindow == windowId && activeTextboxIndex == textBoxId);\
    }\
    itemRect.y += deltaY; labelRect.y += deltaY; valueRect.y += deltaY; }\
    
    #define ADD_STRING(label, ref, toggler) { if (HandleTextboxString(windowId, ++textBoxId, labelRect, label, valueRect, ref, indent, toggler)) {\
        somethingChanged |= !(activeEditorWindow == windowId && activeTextboxIndex == textBoxId);\
    }\
    itemRect.y += deltaY; labelRect.y += deltaY; valueRect.y += deltaY; }
    
    // @TODO this is a monster; refactor!!
    #define ADD_VEC3(label, ref) {{\
        textBoxId++;\
        char valueBuffer[1024];\
        valueBuffer[0] = '\0';\
        \
        strcpy(valueBuffer, formatFloat((ref)->x));\
        strcat(valueBuffer, " , ");\
        strcat(valueBuffer, formatFloat((ref)->y));\
        strcat(valueBuffer, " , ");\
        strcat(valueBuffer, formatFloat((ref)->z));\
        \
        bool *expanded = &shaderEditData.propertiesExpanded[textBoxId];\
        bool tmp = somethingChanged; somethingChanged = false;\
        ADD_STRING(label, valueBuffer, expanded);\
        if (somethingChanged) {\
            Vector3 tmpVec = { 0 };\
            if (sscanf(valueBuffer, " %f , %f , %f ", &tmpVec.x, &tmpVec.y, &tmpVec.z) == 3) {\
                *ref = tmpVec;\
            }\
        }\
        \
        somethingChanged = tmp || somethingChanged;\
        \
        if (!*expanded) {\
            textBoxId += 3;\
        }\
        else {\
            indent += 16;\
            ADD_FLOAT(label " X", &(ref)->x);\
            ADD_FLOAT(label " Y", &(ref)->y);\
            ADD_FLOAT(label " Z", &(ref)->z);\
            indent -= 16;\
        }\
    }}
    
    
    // --------- Smoothness --------
    
    {
        char tmp[1024] = "Hello, world!";
        bool whatever = true;
        ADD_STRING("Foo bar", tmp, &whatever);
    }
    
    if (node->type == SHADER_NODE_OP_ROTATION) {
        
        char valueBuffer[1024];
        valueBuffer[0] = '\0';
        
        // @TODO rename and make consistent
        {
            bool *expanded = &shaderEditData.propertiesExpanded[textBoxId];
            
            Vector3 axis = { node->rotation.x, node->rotation.y, node->rotation.z };
            float angle  = { node->rotation.w };
            
            if (Vector3Length(axis) == 0.0f) {
                axis = (Vector3){ 1, 0, 0 };
                angle = 0.0f;
            }
            valueBuffer[0] = '\0';
            if (fabs(Vector3DotProduct(axis, Vector3One())) == 1.0f) {
                if (axis.x ==  1.) { strcpy(valueBuffer,  "X"); }
                if (axis.y ==  1.) { strcpy(valueBuffer,  "Y"); }
                if (axis.z ==  1.) { strcpy(valueBuffer,  "Z"); }
                if (axis.x == -1.) { strcpy(valueBuffer, "-X"); }
                if (axis.y == -1.) { strcpy(valueBuffer, "-Y"); }
                if (axis.z == -1.) { strcpy(valueBuffer, "-Z"); }
            }
            if (valueBuffer[0] == '\0') {
                strcpy(valueBuffer, formatFloat(axis.x));
                strcat(valueBuffer, " , ");
                strcat(valueBuffer, formatFloat(axis.y));
                strcat(valueBuffer, " , ");
                strcat(valueBuffer, formatFloat(axis.z));
            }
        
            bool tmp = somethingChanged; somethingChanged = false;
            ADD_STRING("Axis", valueBuffer, expanded);
            if (somethingChanged) {
                Vector3 tmpAxis = { 0 };
                bool gotAxis = false;
                // @TODO inefficient
                if (stricmp(valueBuffer,  "x") == 0) { axis = (Vector3){  1,  0,  0 }; gotAxis = true; }
                if (stricmp(valueBuffer,  "y") == 0) { axis = (Vector3){  0,  1,  0 }; gotAxis = true; }
                if (stricmp(valueBuffer,  "z") == 0) { axis = (Vector3){  0,  0,  1 }; gotAxis = true; }
                if (stricmp(valueBuffer, "-x") == 0) { axis = (Vector3){ -1,  0,  0 }; gotAxis = true; }
                if (stricmp(valueBuffer, "-y") == 0) { axis = (Vector3){  0, -1,  0 }; gotAxis = true; }
                if (stricmp(valueBuffer, "-z") == 0) { axis = (Vector3){  0,  0, -1 }; gotAxis = true; }
                
                if (!gotAxis) {
                    if (sscanf(valueBuffer, " %f , %f , %f ", &tmpAxis.x, &tmpAxis.y, &tmpAxis.z) == 3) {
                        axis = tmpAxis;
                    }
                }
                
            }
            somethingChanged = tmp || somethingChanged;
            
            if (!*expanded) {
                textBoxId += 3;
            }
            else {
                indent += 16;
                ADD_FLOAT("Axis X", &axis.x);
                ADD_FLOAT("Axis Y", &axis.y);
                ADD_FLOAT("Axis Z", &axis.z);
                indent -= 16;
            }
            
            angle *= RAD2DEG;
            ++textBoxId;
            ADD_FLOAT("Angle", &angle);
            
            if (somethingChanged) {
                angle *= DEG2RAD;
                node->rotation = (Vector4){ axis.x, axis.y, axis.z, angle };
            }
            
            //somethingChanged = false;
        }
        
    }
    else if (node->type == SHADER_NODE_OP_TRANSLATION) {
        ADD_VEC3("Translation", &node->translation);
    }
    else {
        ADD_FLOAT("Smoothness", &node->smoothness);
    }
    
    // -----------------------------
    
    if (somethingChanged) {
        RebuildShader();
    }
    
    #undef ADD_FLOAT
    #undef ADD_STRING
    #undef ADD_VEC3
    
}

void HandleWindowPropertiesPrimitive() {
    
    int windowId = WINDOW_PROPERTIES;
    
    Rectangle windowRect = editorWindowRects[WINDOW_PROPERTIES];
    Rectangle clientRect = (Rectangle){ windowRect.x + 8, windowRect.y + 30, windowRect.width - 16, windowRect.height - 40 };
    const char *title = TextFormat("Properties - %s", GetShaderNodeName(shaderEditData.selectedIndex));
    
    GuiPanel(windowRect, title);
    
    Rectangle itemRect = clientRect; itemRect.height = 20;
    
    ShaderNodePrimitive* node = (ShaderNodePrimitive*)&shaderNodes[shaderEditData.selectedIndex];
    
    float labelWidth = itemRect.width * 0.5f;
    
    bool somethingChanged = false;
    
    Rectangle labelRect = itemRect; labelRect.width = labelWidth;
    Rectangle valueRect = itemRect; valueRect.x += labelRect.width-1; valueRect.width -= labelRect.width-1;
    float deltaY = labelRect.height + 1;
    
    int textBoxId = 0;
    float indent = 16;
    
    #define ADD_FLOAT(label, ref) { if (HandleTextboxFloat(windowId, ++textBoxId, labelRect, label, valueRect, ref, indent)) {\
        somethingChanged |= !(activeEditorWindow == windowId && activeTextboxIndex == textBoxId);\
    }\
    itemRect.y += deltaY; labelRect.y += deltaY; valueRect.y += deltaY; }\
    
    #define ADD_STRING(label, ref, toggler) { if (HandleTextboxString(windowId, ++textBoxId, labelRect, label, valueRect, ref, indent, toggler)) {\
        somethingChanged |= !(activeEditorWindow == windowId && activeTextboxIndex == textBoxId);\
    }\
    itemRect.y += deltaY; labelRect.y += deltaY; valueRect.y += deltaY; }
    
    // @TODO this is a monster; refactor!!
    #define ADD_VEC3(label, ref) {{\
        textBoxId++;\
        char valueBuffer[1024];\
        valueBuffer[0] = '\0';\
        \
        strcpy(valueBuffer, formatFloat((ref)->x));\
        strcat(valueBuffer, " , ");\
        strcat(valueBuffer, formatFloat((ref)->y));\
        strcat(valueBuffer, " , ");\
        strcat(valueBuffer, formatFloat((ref)->z));\
        \
        bool *expanded = &shaderEditData.propertiesExpanded[textBoxId];\
        bool tmp = somethingChanged; somethingChanged = false;\
        ADD_STRING(label, valueBuffer, expanded);\
        if (somethingChanged) {\
            Vector3 tmpVec = { 0 };\
            if (sscanf(valueBuffer, " %f , %f , %f ", &tmpVec.x, &tmpVec.y, &tmpVec.z) == 3) {\
                *ref = tmpVec;\
            }\
        }\
        \
        somethingChanged = tmp || somethingChanged;\
        \
        if (!*expanded) {\
            textBoxId += 3;\
        }\
        else {\
            indent += 16;\
            ADD_FLOAT(label " X", &(ref)->x);\
            ADD_FLOAT(label " Y", &(ref)->y);\
            ADD_FLOAT(label " Z", &(ref)->z);\
            indent -= 16;\
        }\
    }}
    
    if (node->type == SHADER_NODE_CAPSULE || node->type == SHADER_NODE_CYLINDER) {
        ADD_VEC3("Start point", &node->p1);
        ADD_VEC3("End point", &node->p2);
        /*
        ADD_FLOAT("Start point X", &node->p1.x);
        ADD_FLOAT("Start point Y", &node->p1.y);
        ADD_FLOAT("Start point Z", &node->p1.z);
        ADD_FLOAT("End point X", &node->p2.x);
        ADD_FLOAT("End point Y", &node->p2.y);
        ADD_FLOAT("End point Z", &node->p2.z);
        */
        ADD_FLOAT("Radius", &node->radius);
    }
    else {

        ADD_VEC3("Position", &node->centre);
        
        //ADD_FLOAT("Position X", &node->centre.x);
        //ADD_FLOAT("Position Y", &node->centre.y);
        //ADD_FLOAT("Position Z", &node->centre.z);
        
        if (node->type == SHADER_NODE_SPHERE) {
            ADD_FLOAT("Radius", &node->size.x);
        }
        else {
            ADD_VEC3("Size", &node->size);
            //ADD_FLOAT("Size X", &node->size.x);
            //ADD_FLOAT("Size Y", &node->size.y);
            //ADD_FLOAT("Size Z", &node->size.z);
        }
    }
    
    if (somethingChanged) {
        RebuildShader();
    }
    
    #undef ADD_FLOAT
    #undef ADD_VEC3
}

int HandleTextboxFloat(int windowId, int textBoxId, Rectangle labelBounds, char *label, Rectangle valueBounds, float *value, int indent) {

    Vector2 mousePos = GetMousePosition();
    bool editing = (activeEditorWindow == windowId && activeTextboxIndex == textBoxId);
    bool hovered = (!GuiIsLocked()) && (CheckCollisionPointRec(mousePos, labelBounds) || CheckCollisionPointRec(mousePos, valueBounds));
    
    int result = 0;
    
    if (editing) {
        
        bool dummy = true;
        
        bool finishEditing = false;
        
        if (GuiLabelButtonEx(labelBounds, label, &dummy, indent, 0)) {
            //TraceLog(LOG_INFO, "Label clicked while editing active; finishing editing");
            //finishEditing = true;
        }
        
        if (GuiTextBox(valueBounds, activeTextboxBuffer, 200, true)) {
            TraceLog(LOG_INFO, "Exiting text box; finishing editing");
            finishEditing = true;
        }
        
        if (finishEditing) {
            float newValue;
            if (sscanf(activeTextboxBuffer, "%f", &newValue) == 1) {
                *value = newValue;
            }
            
            activeEditorWindow = WINDOW_PROPERTIES;
            activeTextboxIndex = 0;
            
            result = 1;
        }
    }
    else {
        //static char textBuffer[200] = { 0 };
        
        //int length = sprintf(textBuffer, "%f", *value);    // Basic formatting
        //int length = sprintf(textBuffer, "%- 0g", *value); // [6.9 => " 6.9"], [0.0 => " "]
        //int length = sprintf(textBuffer, *value == 0 ? "% 1f" : "%- 1g", *value);   // [6.9 => " 6.9"], [0.0 => "0"]
        
        //int length = sprintf(textBuffer, "% f", *value);
        //while(length > 0 && textBuffer[length-1] == '0') { textBuffer[--length] = '\0'; }
        //if(length > 0 && textBuffer[length-1] == '.') { textBuffer[--length] = '\0'; }
        char *valueText = formatFloat(*value);
        
        bool activate = false;
        bool dummy = false;
        
        int prevState = GuiGetState();
        GuiSetState(hovered ? STATE_FOCUSED : STATE_NORMAL);
        if (GuiLabelButtonEx(labelBounds, label, &dummy, indent, 0)) {
            activate = true;
        }
        if (GuiLabelButtonEx(valueBounds, valueText, &dummy, 0, 0)) {
            activate = true;
        }
        GuiSetState(prevState);
        
        if (activate) {
            TraceLog(LOG_INFO, "Activating FLOAT field; string = '%s'", valueText);
            TraceLog(LOG_INFO, "> old contents = '%s'", activeTextboxBuffer);
            
            activeEditorWindow = windowId;
            activeTextboxIndex = textBoxId;
            //strcpy(activeTextboxBuffer, textBuffer[0] == ' ' ? textBuffer+1 : textBuffer);
            strcpy(activeTextboxBuffer, valueText);
            textBoxCursorIndex = strlen(valueText);
            
            result = 1;
        }
    }
    
    return result;
}

int HandleTextboxString(int windowId, int textBoxId, Rectangle labelBounds, char *label, Rectangle valueBounds, char *value, int indent, bool *toggler) {

    Vector2 mousePos = GetMousePosition();
    bool editing = (activeEditorWindow == windowId && activeTextboxIndex == textBoxId);
    bool hovered = (!GuiIsLocked()) && (CheckCollisionPointRec(mousePos, labelBounds) || CheckCollisionPointRec(mousePos, valueBounds));
    
    int result = 0;
    
    if (editing) {
        
        bool dummy = true;
        
        bool finishEditing = false;
        
        if (GuiLabelButtonEx(labelBounds, label, &dummy, indent, toggler)) {
            //TraceLog(LOG_INFO, "Label clicked while editing active; finishing editing");
            //finishEditing = true;
        }
        
        if (GuiTextBox(valueBounds, activeTextboxBuffer, 200, true)) {
            TraceLog(LOG_INFO, "Exiting text box; finishing editing");
            finishEditing = true;
        }
        
        if (finishEditing) {
            // @TODO enforce limits on buffer size
            strcpy(value, activeTextboxBuffer);
            
            activeEditorWindow = WINDOW_PROPERTIES;
            activeTextboxIndex = 0;
            
            result = 1;
        }
    }
    else {
        static char textBuffer[200] = { 0 };
        
        strcpy(textBuffer, value);
        
        bool activate = false;
        bool dummy = false;
        
        int prevState = GuiGetState();
        GuiSetState(hovered ? STATE_FOCUSED : STATE_NORMAL);
        if (GuiLabelButtonEx(labelBounds, label, &dummy, indent, toggler)) {
            activate = true;
        }
        if (GuiLabelButtonEx(valueBounds, textBuffer, &dummy, 0, 0)) {
            activate = true;
        }
        GuiSetState(prevState);
        
        if (activate) {
            TraceLog(LOG_INFO, "Activating STRING field; string = '%s'", textBuffer);
            TraceLog(LOG_INFO, "> old contents = '%s'", activeTextboxBuffer);
            
            activeEditorWindow = windowId;
            activeTextboxIndex = textBoxId;
            strcpy(activeTextboxBuffer, textBuffer);
            textBoxCursorIndex = strlen(textBuffer);
            
            result = 1;
        }
    }
    
    return result;
}


int GuiLabelButtonEx(Rectangle bounds, const char *text, bool *active, int indent, bool *toggler) {
    Vector2 mousePos = GetMousePosition();
    bool hovered = (!GuiIsLocked()) && CheckCollisionPointRec(mousePos, bounds);
    *active = *active || (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
    
    int state = GuiGetState();
    int prevState = state;
    
    if (state == STATE_NORMAL) {
        state = *active ? STATE_PRESSED : (hovered ? STATE_FOCUSED : STATE_NORMAL);
    }
    else if (state == STATE_FOCUSED) {
        state = *active ? STATE_PRESSED : STATE_FOCUSED;
    }
    
    GuiSetState(state);
    
    bool dummy = *active;
    if (state == STATE_NORMAL) {
        GuiDrawRectangle(bounds, GuiGetStyle(TEXTBOX, BORDER_WIDTH), GetColor(GuiGetStyle(TEXTBOX, BORDER + (state*3))), BLANK);
    }
    else if (state == STATE_PRESSED) {
        // @TODO sort this out somehow
        // If it's active AND hovered, it should stay active
        // But this is a toggle, so it thinks differently...
        dummy = false;
        GuiToggle(bounds, NULL, &dummy);
    }
    else {
        GuiToggle(bounds, NULL, &dummy);
    }
    //if (state != STATE_NORMAL) {
    //    GuiToggle(bounds, NULL, &dummy);
    //}
    int pressed = hovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    
    float offset = GuiGetStyle(TEXTBOX, BORDER_WIDTH) + GuiGetStyle(TEXTBOX, TEXT_PADDING);
    Rectangle textRect = (Rectangle){ bounds.x + offset + indent, bounds.y, bounds.width - 2 * offset - indent, bounds.height };
    
    Rectangle togglerRect = (Rectangle){ textRect.x - 16, bounds.y + bounds.height * .5 - 16/2, 16, 16 };
    
    bool togglerHot = (toggler != 0) && CheckCollisionPointRec(GetMousePosition(), togglerRect);
    if (togglerHot) {
        if (pressed) {
            *toggler = !*toggler;
            TraceLog(LOG_INFO, "Switching toggler to %d", *toggler);
            pressed = false;
        }
    }
    
    if (pressed) state = STATE_PRESSED;
    
    GuiDrawText(text, textRect, GuiGetStyle(LABEL, TEXT_ALIGNMENT), GetColor(GuiGetStyle(LABEL, TEXT + (state*3))));
    
    if (toggler != 0) {
        Color togglerColour = GetColor(GuiGetStyle(LABEL, TEXT + (state*3)));
        GuiDrawIcon(togglerHot ? (*toggler ? ICON_TREE_COLLAPSE_FILL : ICON_TREE_EXPAND_FILL) : (*toggler ? ICON_TREE_COLLAPSE : ICON_TREE_EXPAND), togglerRect.x, togglerRect.y, 1, togglerColour);
    }
    
    GuiSetState(prevState);
    
    return pressed;
}

void HandleWindowSelectNodeType() {
    Rectangle windowRect = editorWindowRects[WINDOW_SELECT_NODE_TYPE];
    
    if (GuiWindowBox(windowRect, "Add Node")) {
        HideAddNodeDialog();
        return;
    }
    
    Rectangle clientRect = (Rectangle){ windowRect.x + 8, windowRect.y + 30, windowRect.width - 16, windowRect.height - 40 };
    
    bool hot = CheckCollisionPointRec(GetMousePosition(), windowRect);
    bool clicked = hot && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    
    static char buffer[4096];
    buffer[0] = '\0';
    
    int pickedType = -1;
    
    
    // @TODO groups
    for (int group = 0; group < 3; group++) {
        int category;
        switch(group) {
            case 0: category = SHADER_NODE_TYPE_OBJECT;    sprintf(buffer, "Objects");    break;
            case 1: category = SHADER_NODE_TYPE_PRIMITIVE; sprintf(buffer, "Primitives"); break;
            case 2: category = SHADER_NODE_TYPE_OPERATION; sprintf(buffer, "Operations"); break;
            default:
                break;
        }
        
        Rectangle itemRect = clientRect;
        itemRect.height = 20;
        
        GuiLine(itemRect, buffer);
        itemRect.y += itemRect.height + 10;
        buffer[0] = '\0';
        
        int itemsPerRow = 4;
        
        int count = 0;
        for (int i = 0; i < SHADER_NODE_MAX; i++) {
            if (shaderNodeTypeCategory[i] != category) continue;
            sprintf(buffer, "%s%s%s", buffer, ((count % itemsPerRow == 0) ? "\n" : ";"), GuiIconText(GetShaderNodeTypeIcon(i), GetShaderNodeTypeName(i)));
            count++;
        }
        
        itemRect.width = 100; itemRect.height = 60;
        
        int picked = -1;
        GuiToggleGroup(itemRect, buffer+1, &picked);
        
        if (picked != -1) {
            count = 0;
            for (int i = 0; i < SHADER_NODE_MAX; i++) {
                if (shaderNodeTypeCategory[i] != category) continue;
                if (count == picked) {
                    pickedType = i;
                    // (Cannot break here)
                }
                count++;
            }
        }
        
        clientRect.y += 20 + (itemRect.height + 10) * ceilf((float)count / (float)itemsPerRow);
    }
    
    /*
    sprintf(buffer, GuiIconText(GetShaderNodeTypeIcon(0), GetShaderNodeTypeName(0, false)));
    for (int i = 1; i < SHADER_NODE_MAX; i++) {
        sprintf(buffer, "%s%s%s", buffer, ((i % 6 == 0) ? "\n" : ";"), GuiIconText(GetShaderNodeTypeIcon(i), GetShaderNodeTypeName(i, false)));
    }
    
    Rectangle itemRect = clientRect;
    itemRect.width = 100;
    itemRect.height = 60;
    
    int picked = -1; //shaderEditData.selectedIndex == -1 ? -1 : shaderNodes[shaderEditData.selectedIndex].type;
    GuiToggleGroup(itemRect, buffer, &picked);
    */
    
    
    
    if (pickedType != -1) {
        TraceLog(LOG_INFO, "picked = %d", pickedType);
        editorWindowRects[WINDOW_SELECT_NODE_TYPE] = (Rectangle){0,0,0,0};
        activeEditorWindow = WINDOW_VIEWPORT;
        
        if (shaderEditData.selectedIndex != -1) {
            TraceLog(LOG_WARNING, "Addeing new node, to index %d, count is %d", shaderEditData.selectedIndex, shaderNodeCount);
            
            int nodeIndex = DuplicateNode(shaderEditData.selectedIndex, false);
            memset(&shaderNodes[nodeIndex], 0, sizeof(ShaderNode));
            memset(&shaderEditData.nodes[nodeIndex], 0, sizeof(ShaderEditorNodeData));
            shaderNodes[nodeIndex].type = pickedType;
            
            ShaderNodePrimitive *nodeP = (ShaderNodePrimitive*)&shaderNodes[nodeIndex].type;
            ShaderNodeOperation *nodeO = (ShaderNodeOperation*)&shaderNodes[nodeIndex].type;
            
            // Some defaults...
            switch(pickedType) {
                case SHADER_NODE_BOX:       nodeP->size = (Vector3){ 1, 1, 1 }; break;
                case SHADER_NODE_ELLIPSOID: nodeP->size = (Vector3){ 1, 0.5f, 0.5f }; break;
                case SHADER_NODE_SPHERE:    nodeP->size = (Vector3){ 1, 0, 0 }; break;
                case SHADER_NODE_CAPSULE:   nodeP->p2.y = 1.0f; nodeP->radius = 1.0f; break;
                case SHADER_NODE_CYLINDER:  nodeP->p2.y = 1.0f; nodeP->radius = 1.0f; break;
                default:
                    // Don't care...
            }
            
            shaderEditData.nodes[nodeIndex].depth = shaderEditData.nodes[shaderEditData.selectedIndex].depth + 1;
            shaderEditData.selectedIndex = nodeIndex;
            
            RecalcShaderNodesHierarchy();
            ExpandToShowNode(shaderEditData.selectedIndex);
            RebuildShader();
            
            TraceLog(LOG_WARNING, "Added new node, index %d, count is now %d", nodeIndex, shaderNodeCount);
        }
        else {
            TraceLog(LOG_WARNING, "Cannot add new node; nothing selected");
        }
    }
}

char *formatFloat(float value) {
    static char buffer[1024];
    static bool debug = false;
    
    int length = snprintf(buffer, 1023, "%f", value);
    if (debug) { TraceLog(LOG_INFO, "Length = %d,\tText='%s'", length, buffer); }
    
    while(length > 0 && buffer[length-1] == '0') { buffer[--length] = '\0'; }
    if (debug) { TraceLog(LOG_INFO, "Length = %d,\tText='%s'", length, buffer); }
    
    if(length > 0 && buffer[length-1] == '.') { buffer[--length] = '\0'; }
    if (debug) { TraceLog(LOG_INFO, "Length = %d,\tText='%s'", length, buffer); }
    
    debug = false;
    
    return buffer;
}

void ShowAddNodeDialog() {
    if (shaderEditData.selectedIndex == -1) {
        TraceLog(LOG_WARNING, "Trying to add a node but nothing selected!");
        return;
    }
    
    editorWindowRects[WINDOW_SELECT_NODE_TYPE] = (Rectangle){ 0, 0, fmin(640, GetScreenWidth()), fmin(480, GetScreenHeight()) };
    editorWindowRects[WINDOW_SELECT_NODE_TYPE].x = GetScreenWidth()/2 - editorWindowRects[WINDOW_SELECT_NODE_TYPE].width/2;
    editorWindowRects[WINDOW_SELECT_NODE_TYPE].y = GetScreenHeight()/2 - editorWindowRects[WINDOW_SELECT_NODE_TYPE].height/2;
    
    activeEditorWindow = WINDOW_SELECT_NODE_TYPE;
}

void HideAddNodeDialog() {
    editorWindowRects[WINDOW_SELECT_NODE_TYPE] = (Rectangle){0,0,0,0};
    activeEditorWindow = WINDOW_VIEWPORT;
}

// From https://github.com/raysan5/raylib/pull/3709 by luis605
// Get a ray trace from the mouse position within a specific section of the screen
Ray GetViewRay(Vector2 mousePosition, Camera camera, float width, float height)
{
    Ray ray = { 0 };

    // Calculate normalized device coordinates
    // NOTE: y value is negative
    float x = (2.0f*mousePosition.x)/width - 1.0f;
    float y = 1.0f - (2.0f*mousePosition.y)/height;
    float z = 1.0f;

    // Store values in a vector
    Vector3 deviceCoords = { x, y, z };

    // Calculate view matrix from camera look at
    Matrix matView = MatrixLookAt(camera.position, camera.target, camera.up);

    Matrix matProj = MatrixIdentity();

    if (camera.projection == CAMERA_PERSPECTIVE)
    {
        // Calculate projection matrix from perspective
        matProj = MatrixPerspective(camera.fovy*DEG2RAD, ((double)width/(double)height), RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
    }
    else if (camera.projection == CAMERA_ORTHOGRAPHIC)
    {
        double aspect = (double)width/(double)height;
        double top = camera.fovy/2.0;
        double right = top*aspect;

        // Calculate projection matrix from orthographic
        matProj = MatrixOrtho(-right, right, -top, top, 0.01, 1000.0);
    }

    // Unproject far/near points
    Vector3 nearPoint = Vector3Unproject((Vector3){ deviceCoords.x, deviceCoords.y, 0.0f }, matProj, matView);
    Vector3 farPoint = Vector3Unproject((Vector3){ deviceCoords.x, deviceCoords.y, 1.0f }, matProj, matView);

    // Unproject the mouse cursor in the near plane.
    // We need this as the source position because orthographic projects, compared to perspective doesn't have a
    // convergence point, meaning that the "eye" of the camera is more like a plane than a point.
    Vector3 cameraPlanePointerPos = Vector3Unproject((Vector3){ deviceCoords.x, deviceCoords.y, -1.0f }, matProj, matView);

    // Calculate normalized direction vector
    Vector3 direction = Vector3Normalize(Vector3Subtract(farPoint, nearPoint));

    if (camera.projection == CAMERA_PERSPECTIVE) ray.position = camera.position;
    else if (camera.projection == CAMERA_ORTHOGRAPHIC) ray.position = cameraPlanePointerPos;

    // Apply calculated vectors to ray
    ray.direction = direction;

    return ray;
}