# Computer Graphics 2026 - Rubik cube project

**Student Information**
* **Name:** [Larysa Nechay]
* **Student ID:** [337575559]

#Project report:

## Overview

In this project I created a Rubik cube simulator that you can solve. This simulator also includes 3 different Rubik cube modes, where you can decide if you want to play with a regular cube, neon cube or glass cube. It also has lighting controls, camera controls, rotation controls and shading controls for fully enjoyable experience:)
It is important to mention that majority of the basic functions related to drawing, rotations, camera and lighting are taken from the assignments of the course, so this report may skip over some of the explanations about them.

## Cube creation

I started the project by creating the Rubik cube object, and this section will solely talk about that process.

The cube isn't just a solid object like the ones that we saw in several assignments, instead it is procedurally constructed as 26 visible cubies. Its puzzle state is persistent and its geometric faces are generated at draw time.

**Key implementation detail:** The grid's center is intentionally omitted. Each visible piece retains sticker colors as it moves; stickers are not repainted based on the cubie's new position.

### `rubiks_preview.h` — lines 24–55

```
struct RubiksCubie {//defines a group of related state fields
    glm::ivec3 grid; // position in the cube
    glm::mat3 orientation; //rotation matrix of the cubie's rotation
    uint32_t stickers[6];//colors of the cubie stickers
};

inline std::vector<RubiksCubie>& rubiks_cubies() {// this function is for storing the cube changes
    static std::vector<RubiksCubie> pieces = [] {// static vector of our cubies
        std::vector<RubiksCubie> result;
        const uint32_t colors[6] = {// colors of stickers
            MFB_RGB(210,35,35), MFB_RGB(245,125,20),
            MFB_RGB(245,245,245), MFB_RGB(245,210,25),
            MFB_RGB(25,170,70), MFB_RGB(35,85,215)
        };
        for (int x=-1; x<=1; ++x)//we loop through every cubie except the middle one
        for (int y=-1; y<=1; ++y)
        for (int z=-1; z<=1; ++z) {
            if (x==0 && y==0 && z==0) continue;
            RubiksCubie p{};
            p.grid = glm::ivec3(x,y,z);//position is the coordinates
            p.orientation = glm::mat3(1.0f);//I matrix
            p.stickers[0] = x== 1 ? colors[0] : 0;//attaching colors
            p.stickers[1] = x==-1 ? colors[1] : 0;
            p.stickers[2] = y== 1 ? colors[2] : 0;
            p.stickers[3] = y==-1 ? colors[3] : 0;
            p.stickers[4] = z== 1 ? colors[4] : 0;
            p.stickers[5] = z==-1 ? colors[5] : 0;
            result.push_back(p);//stores the cubie
        }
        return result;
    }();
    return pieces;
```
This code creates and stores the 26 individual cubies that make up the Rubik’s Cube. Each cubie has a 3D grid position, an orientation matrix to track its rotations, and six possible sticker colors.
Three nested loops generate a 3×3×3 arrangement, skipping the invisible center cubie. Each cubie starts with an identity rotation matrix, and colors are assigned only to its outward-facing sides.
Finally, all cubies are stored in a static vector, which is initialized only once and preserves their positions and orientations as the cube is rotated.

### `rubiks_preview.h` — lines 58–82

```
inline void rubiks_reset() {
    // Reinitialize from the solved configuration explicitly.
    rubiks_animation().active = false;
    auto& pieces = rubiks_cubies();
    const uint32_t colors[6] = {
        MFB_RGB(210,35,35), MFB_RGB(245,125,20),
        MFB_RGB(245,245,245), MFB_RGB(245,210,25),
        MFB_RGB(25,170,70), MFB_RGB(35,85,215)
    };
    pieces.clear();//clears all of the pieces
    for (int x=-1; x<=1; ++x)
    for (int y=-1; y<=1; ++y)
    for (int z=-1; z<=1; ++z) {
        if (x==0 && y==0 && z==0) continue;
        RubiksCubie p{};
        p.grid = glm::ivec3(x,y,z);//position is the coorsinates
        p.orientation = glm::mat3(1.0f);// rotation matrix is I
        p.stickers[0] = x== 1 ? colors[0] : 0;//assigning colors
        p.stickers[1] = x==-1 ? colors[1] : 0;
        p.stickers[2] = y== 1 ? colors[2] : 0;
        p.stickers[3] = y==-1 ? colors[3] : 0;
        p.stickers[4] = z== 1 ? colors[4] : 0;
        p.stickers[5] = z==-1 ? colors[5] : 0;
        pieces.push_back(p);
    }
```
This function resets the Rubik’s Cube to its original solved state. First, it stops any active rotation animation and clears the existing cubies. Then, using three nested loops, it recreates all 26 cubies in their original grid positions, resets their orientation matrices to identity matrices, and reassigns the correct sticker colors to their outward-facing sides. Finally, the recreated cubies are stored in the vector, restoring the cube to its starting configuration.

### `rubiks_preview.h` — lines 223–240

```
  //We start by modeling the small cubes in the rubik cube
  //the first line we define the 6 sides/faces of the little cubes
    const glm::vec3 normals[6] = {{1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}};
  //then we define the 6 colors of the cubes
    const uint32_t colors[6] = {MFB_RGB(210,35,35), MFB_RGB(245,125,20),MFB_RGB(245,245,245), MFB_RGB(245,210,25),MFB_RGB(25,170,70), MFB_RGB(35,85,215)};
    // Face corners are ordered consistently around each outward normal.
    const glm::vec3 corners[6][4] = {//for each face there are 4 vertices
        {{1,-1,-1},{1,1,-1},{1,1,1},{1,-1,1}},
        {{-1,-1,1},{-1,1,1},{-1,1,-1},{-1,-1,-1}},
        {{-1,1,-1},{-1,1,1},{1,1,1},{1,1,-1}},
        {{-1,-1,1},{-1,-1,-1},{1,-1,-1},{1,-1,1}},
        {{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}},
        {{1,-1,-1},{-1,-1,-1},{-1,1,-1},{1,1,-1}}
    };
    constexpr float spacing = 230.0f;//distance between the centers of neighboring cubies
    constexpr float halfSize = 107.0f;//half the width of a cubie
    constexpr float stickerHalf = 89.0f;//half the width of a sticker
    constexpr float stickerOffset = 108.0f;//distance of the sticker plane from its cubie center
```
This code defines the geometry and appearance of each small cubie in the Rubik’s Cube. It specifies the six face normals (directions), the six sticker colors, and the four vertices that form each face. It also defines the spacing between cubies, their size, the size of the stickers, and how far the stickers are positioned from the cubie centers. Together, these values determine how the individual cubies and their colored faces are constructed and displayed.

### `rubiks_preview.h` — lines 293–296

```
    auto quad = [&](const glm::vec3 v[4], uint32_t color) {
        triangle(v[0], v[1], v[2], color);
        triangle(v[0], v[2], v[3], color);
    };
```
This code defines a helper function called quad, which takes four vertices and a color and divides a square face into two triangles. Both triangles are drawn using the triangle() function, allowing our triangle-based software renderer to display the square faces of the Rubik’s Cube.

### `rubiks_preview.h` — lines 658–710

```
    for (const auto& p : rubiks_cubies()) {

        glm::vec3 center = glm::vec3(p.grid) * spacing;
        glm::mat3 orientation = p.orientation;

        // Only rotate cubies belonging to the moving layer
        if (animation.active && p.grid[animAxis] == animLayer) {
            center = animatedRotation * center;
            orientation = animatedRotation * orientation;
        }

        for (int f = 0; f < 6; ++f) {

    // Glass Mode: skip faces pointing away from the camera
    if (glassMode && !neonMode) {
        glm::vec3 worldNormal = orientation * normals[f];

        glm::vec3 faceCenter =
            center + worldNormal * halfSize;

        glm::vec3 cameraCenter = glm::vec3(
            view * final_matrix * glm::vec4(faceCenter, 1.0f)
        );

        glm::vec3 cameraNormal = glm::normalize(
            glm::mat3(view * final_matrix) * worldNormal
        );

        glm::vec3 toCamera = glm::normalize(-cameraCenter);

        if (glm::dot(cameraNormal, toCamera) <= 0.0f)
            continue;
    }

    glm::vec3 body[4];
    for (int k = 0; k < 4; ++k)
        body[k] = center + orientation * (corners[f][k] * halfSize);
            
            if (glassMode && !neonMode) {
                collectGlassFace(body,MFB_RGB(120, 190, 225),0.23f,false);
            } else {
                    quad(body, MFB_RGB(20, 22, 28));
            }

            if (p.stickers[f] == 0) continue;
            glm::vec3 sticker[4];
            for (int k=0; k<4; ++k) {
                glm::vec3 v = corners[f][k];
                sticker[k] = center + orientation *
                    ((v - normals[f])*stickerHalf + normals[f]*stickerOffset);
            }
            
            
```
This code loops through every cubie and constructs its visible faces, using each cubie's position and orientation. If a rotation animation is active, only the cubies belonging to the moving layer are temporarily rotated to create smooth movement. For each cubie, the code generates its six faces. In Glass Mode, faces pointing away from the camera are skipped, and visible faces are collected for transparent rendering. Otherwise, the cubie bodies are drawn normally using two triangles per face. Finally, the code checks which faces have colored stickers and calculates their four vertices, positioning them slightly above the cubie surface.

## 2. Connecting the cube to rotation mechanics

A face turn changes both the integer grid coordinates and the orientation matrix of the selected outer-layer cubies.

**Key implementation detail:** The quarter-turn matrix is applied to both grid position and orientation. Snapping orientation entries to integers avoids cumulative drift.

### `rubiks_preview.h` — lines 85–119

```cpp
// axis: 0=X, 1=Y, 2=Z; layer: -1 or +1; direction: +/-1
inline void rubiks_rotate_layer(int axis, int layer, int direction) {
    if (axis < 0 || axis > 2 || (layer != -1 && layer != 1)) return;//if we get not established moves return
    direction = direction >= 0 ? 1 : -1;
    glm::vec3 axisVector(0.0f);
    axisVector[axis] = 1.0f;// we enter 1 in the vector to the axis where we want to rotate
    glm::mat3 rotation = glm::mat3(glm::rotate(glm::mat4(1.0f),
        glm::radians(90.0f * direction), axisVector));//rotating using past rotation function
    for (auto& p : rubiks_cubies()) {
        if (p.grid[axis] != layer) continue;
        glm::vec3 moved = rotation * glm::vec3(p.grid);
        p.grid = glm::ivec3(glm::round(moved));
        p.orientation = rotation * p.orientation;
        // Snap orientation to exact axis-aligned values to prevent drift.
        for (int c=0; c<3; ++c)
            for (int r=0; r<3; ++r)
                p.orientation[c][r] = std::round(p.orientation[c][r]);
    }
}

// Clockwise when looking directly at the named face.
inline void rubiks_apply_turn(char face, bool inverse) {// the keyboard keys turn into comands
    int axis=0, layer=1;
    switch (face) {
        case 'R': case 'r': axis=0; layer= 1; break;
        case 'L': case 'l': axis=0; layer=-1; break;
        case 'U': case 'u': axis=1; layer= 1; break;
        case 'D': case 'd': axis=1; layer=-1; break;
        case 'F': case 'f': axis=2; layer= 1; break;
        case 'B': case 'b': axis=2; layer=-1; break;
        default: return;
    }
    int direction = -layer; // clockwise as viewed from outside the face
    if (inverse) direction = -direction;
    rubiks_rotate_layer(axis, layer, direction);
```



### `rubiks_preview.h` — lines 122–146

```cpp
inline void rubiks_turn(char face, bool inverse = false) {
    auto& animation = rubiks_animation();

    // Ignore additional moves during an animation
    if (animation.active)
        return;

    animation.active = true;
    animation.face = face;
    animation.inverse = inverse;
    animation.elapsed = 0.0f;
}

inline void rubiks_update(float delta_time) {
    auto& animation = rubiks_animation();

    if (!animation.active)
        return;

    animation.elapsed += delta_time;

    if (animation.elapsed >= animation.duration) {
        animation.active = false;
        rubiks_apply_turn(animation.face, animation.inverse);
    }
```

## 3. Cube axes, whole-cube rotation and UI bridge

Whole-cube transformations change the displayed model, whereas face-turn callbacks change the puzzle state. The UI bridge connects keyboard and mouse events to both systems.

**Key implementation detail:** The bridge intentionally uses callbacks for puzzle turns and pointers for continuously editable transform values. The uploaded bridge comments out generic L/R selection to reserve those keys for cube face turns.

### `main.cpp` — lines 38–62

```cpp
// HW2 Part 4: Local transformations
static float local_translation_x = 0.0f;
static float local_translation_y = 0.0f;
static float local_translation_z = 0.0f;

static float local_rotation_x = 0.0f;
static float local_rotation_y = 0.0f;
static float local_rotation_z = 0.0f;

static float local_scale_x = 1.0f;
static float local_scale_y = 1.0f;
static float local_scale_z = 1.0f;

// HW2 Part 4: World transformations
static float world_translation_x = 0.0f;
static float world_translation_y = 0.0f;
static float world_translation_z = 0.0f;

static float world_rotation_x = 0.0f;
static float world_rotation_y = 0.0f;
static float world_rotation_z = 0.0f;

static float world_scale_x = 1.0f;
static float world_scale_y = 1.0f;
static float world_scale_z = 1.0f;
```


### `main.cpp` — lines 563–595

```cpp
{
    float rubiksEffectTime = 0.0f;
    // Connect the UI bridge to the Rubik's Cube
    ui_bridge_bind_rubiks(
        [](char face, bool inverse) {
            rubiks_turn(face, inverse);
        },
        []() {
            rubiks_reset();
        }
    );
    ui_bridge_bind_transformations(
    &local_translation_x,
    &local_translation_y,
    &local_translation_z,

    &local_rotation_x,
    &local_rotation_y,
    &local_rotation_z,

    &local_scale_x,
    &local_scale_y,
    &local_scale_z,

    &world_translation_x,
    &world_translation_y,
    &world_translation_z,

    &world_rotation_x,
    &world_rotation_y,
    &world_rotation_z,

    &world_scale_x,
```



### `main.cpp` — lines 714–765

```cpp
    // local transformation matrices

    glm::mat4 local_scale_matrix = glm::scale(glm::mat4(1.0f),glm::vec3(local_scale_x, local_scale_y, local_scale_z));
    glm::mat4 local_rotation_matrix = glm::mat4(1.0f);

    local_rotation_matrix = glm::rotate(local_rotation_matrix,glm::radians(local_rotation_x),glm::vec3(1.0f, 0.0f, 0.0f));
    local_rotation_matrix = glm::rotate(local_rotation_matrix,glm::radians(local_rotation_y),glm::vec3(0.0f, 1.0f, 0.0f));
    local_rotation_matrix = glm::rotate(local_rotation_matrix,glm::radians(local_rotation_z),glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 local_translation_matrix = glm::translate(glm::mat4(1.0f),glm::vec3(local_translation_x,local_translation_y,local_translation_z));

    glm::mat4 local_matrix =local_translation_matrix *local_rotation_matrix *local_scale_matrix;

    // world transformation matrices

    glm::mat4 world_scale_matrix = glm::scale(glm::mat4(1.0f),glm::vec3(world_scale_x, world_scale_y, world_scale_z));

    glm::mat4 world_rotation_matrix = glm::mat4(1.0f);
    world_rotation_matrix = glm::rotate(world_rotation_matrix,glm::radians(world_rotation_x),glm::vec3(1.0f, 0.0f, 0.0f));
    world_rotation_matrix = glm::rotate(world_rotation_matrix,glm::radians(world_rotation_y),glm::vec3(0.0f, 1.0f, 0.0f));
    world_rotation_matrix = glm::rotate(world_rotation_matrix,glm::radians(world_rotation_z),glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 world_translation_matrix = glm::translate(glm::mat4(1.0f),glm::vec3(world_translation_x,world_translation_y,world_translation_z));

    glm::mat4 world_matrix =world_translation_matrix *world_rotation_matrix *world_scale_matrix;
    glm::mat4 final_matrix =world_matrix * local_matrix;
    glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(final_matrix)));

// ----------------------View Matrix-----------------------

// Camera translation must be inverted
    glm::mat4 view_translation = glm::translate(glm::mat4(1.0f),-camera.position);

// Camera rotation must also be inverted
    glm::mat4 view_rotation = glm::mat4(1.0f);

    view_rotation = glm::rotate(view_rotation, glm::radians(-camera.rotation.z),glm::vec3(0.0f, 0.0f, 1.0f));
    view_rotation = glm::rotate(view_rotation,glm::radians(-camera.rotation.y),glm::vec3(0.0f, 1.0f, 0.0f));
    view_rotation = glm::rotate(view_rotation,glm::radians(-camera.rotation.x),glm::vec3(1.0f, 0.0f, 0.0f));

    glm::mat4 view_matrix =view_rotation * view_translation;

// ---------------- Perspective Projection ----------------

    float fov = 60.0f;
    float aspect_ratio = (float)WIDTH / (float)HEIGHT;
    float near_plane = 0.1f;
    float far_plane = 5000.0f;

    glm::mat4 perspective_matrix =glm::perspective(glm::radians(fov),aspect_ratio,near_plane,far_plane);

      
```



### `main.cpp` — lines 802–824

```cpp
    if (mu_begin_window(ctx, "Local Rotation", mu_rect(20, 20, 340, 260))) {
        int w[] = {-1};
        mu_layout_row(ctx, 1, w, 0);
        mu_label(ctx, "Local Rotation X");
        mu_slider(ctx, &local_rotation_x, -180.0f, 180.0f);
        mu_label(ctx, "Local Rotation Y");
        mu_slider(ctx, &local_rotation_y, -180.0f, 180.0f);
        mu_label(ctx, "Local Rotation Z");
        mu_slider(ctx, &local_rotation_z, -180.0f, 180.0f);
        mu_end_window(ctx);
    }

    if (mu_begin_window(ctx, "Camera", mu_rect(20, 300, 340, 380))) {
        int w[] = {-1};
        mu_layout_row(ctx, 1, w, 0);
        mu_label(ctx, "Camera Position X");
        mu_slider(ctx, &camera.position.x, -500.0f, 500.0f);
        mu_label(ctx, "Camera Position Y");
        mu_slider(ctx, &camera.position.y, -500.0f, 500.0f);
        mu_label(ctx, "Camera Position Z");
        mu_slider(ctx, &camera.position.z, -500.0f, 5000.0f);
        mu_end_window(ctx);
    }
```

### `ui_bridge.h` — lines 14–48

```cpp
// Which coordinate frame is selected?
enum TransformFrame {
    FRAME_LOCAL,
    FRAME_WORLD
};

// Which transformation is selected?
enum TransformType {
    TRANSFORM_TRANSLATION,
    TRANSFORM_ROTATION,
    TRANSFORM_SCALE
};

// Which axis is selected?
enum TransformAxis {
    AXIS_X,
    AXIS_Y,
    AXIS_Z
};

// Callback type for Rubik's Cube commands
using RubiksTurnCallback = void (*)(char face, bool inverse);
using RubiksResetCallback = void (*)();

// Functions provided by main.cpp
static RubiksTurnCallback rubiks_turn_callback = nullptr;
static RubiksResetCallback rubiks_reset_callback = nullptr;

// Connect the UI bridge to the cube
inline void ui_bridge_bind_rubiks(
    RubiksTurnCallback turn,
    RubiksResetCallback reset
) {
    rubiks_turn_callback = turn;
    rubiks_reset_callback = reset;
```


### `ui_bridge.h` — lines 51–179

```cpp
// Current keyboard selections
static TransformFrame selected_frame = FRAME_LOCAL;
static TransformType selected_transform = TRANSFORM_TRANSLATION;
static TransformAxis selected_axis = AXIS_X;


// Pointers to transformation variables from main.cpp
static float* p_local_translation_x = nullptr;
static float* p_local_translation_y = nullptr;
static float* p_local_translation_z = nullptr;

static float* p_local_rotation_x = nullptr;
static float* p_local_rotation_y = nullptr;
static float* p_local_rotation_z = nullptr;

static float* p_local_scale_x = nullptr;
static float* p_local_scale_y = nullptr;
static float* p_local_scale_z = nullptr;

static float* p_world_translation_x = nullptr;
static float* p_world_translation_y = nullptr;
static float* p_world_translation_z = nullptr;

static float* p_world_rotation_x = nullptr;
static float* p_world_rotation_y = nullptr;
static float* p_world_rotation_z = nullptr;

static float* p_world_scale_x = nullptr;
static float* p_world_scale_y = nullptr;
static float* p_world_scale_z = nullptr;


// Connect the variables from main.cpp to this input bridge
inline void ui_bridge_bind_transformations(
    float* local_tx,
    float* local_ty,
    float* local_tz,

    float* local_rx,
    float* local_ry,
    float* local_rz,

    float* local_sx,
    float* local_sy,
    float* local_sz,

    float* world_tx,
    float* world_ty,
    float* world_tz,

    float* world_rx,
    float* world_ry,
    float* world_rz,

    float* world_sx,
    float* world_sy,
    float* world_sz
) {
    p_local_translation_x = local_tx;
    p_local_translation_y = local_ty;
    p_local_translation_z = local_tz;

    p_local_rotation_x = local_rx;
    p_local_rotation_y = local_ry;
    p_local_rotation_z = local_rz;

    p_local_scale_x = local_sx;
    p_local_scale_y = local_sy;
    p_local_scale_z = local_sz;

    p_world_translation_x = world_tx;
    p_world_translation_y = world_ty;
    p_world_translation_z = world_tz;

    p_world_rotation_x = world_rx;
    p_world_rotation_y = world_ry;
    p_world_rotation_z = world_rz;

    p_world_scale_x = world_sx;
    p_world_scale_y = world_sy;
    p_world_scale_z = world_sz;
}


// Return the currently selected variable
inline float* get_selected_transform_value() {

    if (selected_frame == FRAME_LOCAL) {

        if (selected_transform == TRANSFORM_TRANSLATION) {
            if (selected_axis == AXIS_X) return p_local_translation_x;
            if (selected_axis == AXIS_Y) return p_local_translation_y;
            return p_local_translation_z;
        }

        if (selected_transform == TRANSFORM_ROTATION) {
            if (selected_axis == AXIS_X) return p_local_rotation_x;
            if (selected_axis == AXIS_Y) return p_local_rotation_y;
            return p_local_rotation_z;
        }

        if (selected_transform == TRANSFORM_SCALE) {
            if (selected_axis == AXIS_X) return p_local_scale_x;
            if (selected_axis == AXIS_Y) return p_local_scale_y;
            return p_local_scale_z;
        }

    } else {

        if (selected_transform == TRANSFORM_TRANSLATION) {
            if (selected_axis == AXIS_X) return p_world_translation_x;
            if (selected_axis == AXIS_Y) return p_world_translation_y;
            return p_world_translation_z;
        }

        if (selected_transform == TRANSFORM_ROTATION) {
            if (selected_axis == AXIS_X) return p_world_rotation_x;
            if (selected_axis == AXIS_Y) return p_world_rotation_y;
            return p_world_rotation_z;
        }

        if (selected_transform == TRANSFORM_SCALE) {
            if (selected_axis == AXIS_X) return p_world_scale_x;
            if (selected_axis == AXIS_Y) return p_world_scale_y;
            return p_world_scale_z;
        }
    }

    return nullptr;
```



### `ui_bridge.h` — lines 183–237

```cpp
inline void ui_bridge_char_input(struct mfb_window* window, unsigned int codepoint) {
    (void)window;
    if (codepoint < 0x80 && g_pending_text_len < (int)sizeof(g_pending_text) - 1) {
        g_pending_text[g_pending_text_len++] = (char)codepoint;
        g_pending_text[g_pending_text_len] = '\0';
    }
}

inline void ui_bridge_input(mu_Context* ctx, struct mfb_window* window) {
    if (g_pending_text_len > 0) {
        mu_input_text(ctx, g_pending_text);
        g_pending_text_len = 0;
        g_pending_text[0] = '\0';
    }

    // Mouse Position
    int mx = mfb_get_mouse_x(window);
    int my = mfb_get_mouse_y(window);

    mu_input_mousemove(ctx, mx, my);


    // Mouse Buttons — only fire down/up on state transitions
    static uint8_t prev_mouse[8] = {};
    const uint8_t* mouse_btn = mfb_get_mouse_button_buffer(window);
    
    auto sync_mouse = [&](int mfb_btn, int mu_btn) {
        uint8_t cur = mouse_btn[mfb_btn];
        if (cur && !prev_mouse[mfb_btn])  mu_input_mousedown(ctx, mx, my, mu_btn);
        if (!cur && prev_mouse[mfb_btn])  mu_input_mouseup  (ctx, mx, my, mu_btn);
        prev_mouse[mfb_btn] = cur;
    };
    sync_mouse(MFB_MOUSE_LEFT,   MU_MOUSE_LEFT);
    sync_mouse(MFB_MOUSE_RIGHT,  MU_MOUSE_RIGHT);
    sync_mouse(MFB_MOUSE_MIDDLE, MU_MOUSE_MIDDLE);

    // Keyboard — only fire down/up on state transitions
    static uint8_t prev_keys[MFB_KB_KEY_LAST + 1] = {};
    const uint8_t* keys = mfb_get_key_buffer(window);

    auto sync_key = [&](int mfb_key, int mu_key) {
        uint8_t cur = keys[mfb_key];
        if (cur && !prev_keys[mfb_key])  mu_input_keydown(ctx, mu_key);
        if (!cur && prev_keys[mfb_key])  mu_input_keyup  (ctx, mu_key);
        prev_keys[mfb_key] = cur;
    };
    sync_key(MFB_KB_KEY_LEFT_SHIFT,    MU_KEY_SHIFT);
    sync_key(MFB_KB_KEY_RIGHT_SHIFT,   MU_KEY_SHIFT);
    sync_key(MFB_KB_KEY_LEFT_CONTROL,  MU_KEY_CTRL);
    sync_key(MFB_KB_KEY_RIGHT_CONTROL, MU_KEY_CTRL);
    sync_key(MFB_KB_KEY_LEFT_ALT,      MU_KEY_ALT);
    sync_key(MFB_KB_KEY_RIGHT_ALT,     MU_KEY_ALT);
    sync_key(MFB_KB_KEY_ENTER,         MU_KEY_RETURN);
    sync_key(MFB_KB_KEY_KP_ENTER,      MU_KEY_RETURN);
    sync_key(MFB_KB_KEY_BACKSPACE,     MU_KEY_BACKSPACE);
```



### `ui_bridge.h` — lines 239–330

```cpp
// ========================================================
    // PART 6 - APPROACH 1
    // Keyboard command system
    //
    // L/G -> frame
    // T/R/S -> transformation
    // X/Y/Z -> axis
    // Left/Right arrows -> decrease/increase
    // ========================================================


    // L = Local
    //if (keys[MFB_KB_KEY_L] && !prev_keys[MFB_KB_KEY_L]) {
    //    selected_frame = FRAME_LOCAL;
    //}

    // G = Global / World
    if (keys[MFB_KB_KEY_G] && !prev_keys[MFB_KB_KEY_G]) {
        selected_frame = FRAME_WORLD;
    }


    // T = Translation
    if (keys[MFB_KB_KEY_T] && !prev_keys[MFB_KB_KEY_T]) {
        selected_transform = TRANSFORM_TRANSLATION;
    }

    // R = Rotation
    //if (keys[MFB_KB_KEY_R] && !prev_keys[MFB_KB_KEY_R]) {
    //    selected_transform = TRANSFORM_ROTATION;
   // }

    // S = Scale
    if (keys[MFB_KB_KEY_S] && !prev_keys[MFB_KB_KEY_S]) {
        selected_transform = TRANSFORM_SCALE;
    }


    // Axis selection
    if (keys[MFB_KB_KEY_X] && !prev_keys[MFB_KB_KEY_X]) {
        selected_axis = AXIS_X;
    }

    if (keys[MFB_KB_KEY_Y] && !prev_keys[MFB_KB_KEY_Y]) {
        selected_axis = AXIS_Y;
    }

    if (keys[MFB_KB_KEY_Z] && !prev_keys[MFB_KB_KEY_Z]) {
        selected_axis = AXIS_Z;
    }


    // Determine step size
    float keyboard_step = 1.0f;

    if (selected_transform == TRANSFORM_TRANSLATION)
        keyboard_step = 10.0f;

    if (selected_transform == TRANSFORM_ROTATION)
        keyboard_step = 5.0f;

    if (selected_transform == TRANSFORM_SCALE)
        keyboard_step = 0.1f;


    float* selected_value = get_selected_transform_value();


    // Left arrow = decrease
    if (selected_value &&
        keys[MFB_KB_KEY_LEFT] &&
        !prev_keys[MFB_KB_KEY_LEFT]) {

        *selected_value -= keyboard_step;

        // Do not allow negative/zero scale
        if (selected_transform == TRANSFORM_SCALE &&
            *selected_value < 0.1f) {

            *selected_value = 0.1f;
        }
    }


    // Right arrow = increase
    if (selected_value &&
        keys[MFB_KB_KEY_RIGHT] &&
        !prev_keys[MFB_KB_KEY_RIGHT]) {

        *selected_value += keyboard_step;
    }

```



### `ui_bridge.h` — lines 332–519

```cpp
// ========================================================
// PART 6 - APPROACH 2
// Direct mouse manipulation
//
// Left mouse  = Local
// Right mouse = World
//
// Shift + drag:
// horizontal = Translation X
// vertical   = Translation Y
//
// Shift + 1 + horizontal drag:
// Translation Z
//
// Ctrl + drag:
// horizontal = Rotation Y
// vertical   = Rotation X
//
// Ctrl + mouse wheel:
// Rotation Z
//
// Mouse wheel:
// Uniform scaling
// ========================================================


    bool left_down =mouse_btn[MFB_MOUSE_LEFT];

    bool right_down =mouse_btn[MFB_MOUSE_RIGHT];


    bool shift_down =keys[MFB_KB_KEY_LEFT_SHIFT] ||keys[MFB_KB_KEY_RIGHT_SHIFT];

    bool ctrl_down =keys[MFB_KB_KEY_LEFT_CONTROL] ||keys[MFB_KB_KEY_RIGHT_CONTROL];

    bool one_down = keys[MFB_KB_KEY_1];


    // Remember last mouse position
    static int previous_mx = mx;
    static int previous_my = my;

    int dx = mx - previous_mx;
    int dy = my - previous_my;


    // Which frame does the mouse control?
    TransformFrame mouse_frame = selected_frame;

    if (left_down)
        mouse_frame = FRAME_LOCAL;

    if (right_down)
        mouse_frame = FRAME_WORLD;


    // --------------------------------------------------------
    // Mouse translation
    // --------------------------------------------------------



// Translation only happens while Shift is pressed.
if ((left_down || right_down) && shift_down && !ctrl_down) {

    // Shift + 1 + horizontal drag = Z translation
    if (one_down) {

        if (mouse_frame == FRAME_LOCAL) {

            if (p_local_translation_z)
                *p_local_translation_z += dx;

        } else {

            if (p_world_translation_z)
                *p_world_translation_z += dx;
        }

    } else {

        // Shift + drag = X/Y translation
        if (mouse_frame == FRAME_LOCAL) {

            if (p_local_translation_x)
                *p_local_translation_x += dx;

            if (p_local_translation_y)
                *p_local_translation_y += dy;

        } else {

            if (p_world_translation_x)
                *p_world_translation_x += dx;

            if (p_world_translation_y)
                *p_world_translation_y += dy;
        }
    }
}


// --------------------------------------------------------
// Mouse rotation
// --------------------------------------------------------

// Ctrl + drag:
// horizontal = Y rotation
// vertical   = X rotation
if ((left_down || right_down) && ctrl_down) {

    const float rotation_speed = 0.5f;

    if (mouse_frame == FRAME_LOCAL) {

        if (p_local_rotation_y)
            *p_local_rotation_y += dx * rotation_speed;

        if (p_local_rotation_x)
            *p_local_rotation_x += dy * rotation_speed;

    } else {

        if (p_world_rotation_y)
            *p_world_rotation_y += dx * rotation_speed;

        if (p_world_rotation_x)
            *p_world_rotation_x += dy * rotation_speed;
    }
}

    previous_mx = mx;
    previous_my = my;


    // ========================================================
    // Mouse wheel = uniform scaling
    // ========================================================

    float scroll_y = mfb_get_mouse_scroll_y(window);

    if (scroll_y != 0) {

    mu_input_scroll(ctx, 0, (int)(scroll_y * -10));

    // Ctrl + mouse wheel = Z rotation
    if (ctrl_down) {

        float rotation_change = scroll_y * 5.0f;

        if (selected_frame == FRAME_LOCAL) {
            if (p_local_rotation_z)
                *p_local_rotation_z += rotation_change;
        }
        else {
            if (p_world_rotation_z)
                *p_world_rotation_z += rotation_change;
        }
    }

    // Normal mouse wheel = uniform scale
    else {

        float scale_change = scroll_y * 0.1f;

        if (selected_frame == FRAME_LOCAL) {

            *p_local_scale_x += scale_change;
            *p_local_scale_y += scale_change;
            *p_local_scale_z += scale_change;

            if (*p_local_scale_x < 0.1f) *p_local_scale_x = 0.1f;
            if (*p_local_scale_y < 0.1f) *p_local_scale_y = 0.1f;
            if (*p_local_scale_z < 0.1f) *p_local_scale_z = 0.1f;
        }
        else {

            *p_world_scale_x += scale_change;
            *p_world_scale_y += scale_change;
            *p_world_scale_z += scale_change;

            if (*p_world_scale_x < 0.1f) *p_world_scale_x = 0.1f;
            if (*p_world_scale_y < 0.1f) *p_world_scale_y = 0.1f;
            if (*p_world_scale_z < 0.1f) *p_world_scale_z = 0.1f;
        }
    }
}

```



### `ui_bridge.h` — lines 520–562

```cpp
// ========================================================
// RUBIK'S CUBE KEYBOARD CONTROLS
// ========================================================

bool cube_reverse =
    keys[MFB_KB_KEY_LEFT_SHIFT] ||
    keys[MFB_KB_KEY_RIGHT_SHIFT];

auto cube_key = [&](int key, char face) {
    if (keys[key] && !prev_keys[key]) {
        if (rubiks_turn_callback) {
            rubiks_turn_callback(face, cube_reverse);
        }
    }
};

cube_key(MFB_KB_KEY_R, 'R');
cube_key(MFB_KB_KEY_L, 'L');
cube_key(MFB_KB_KEY_U, 'U');
cube_key(MFB_KB_KEY_D, 'D');
cube_key(MFB_KB_KEY_F, 'F');
cube_key(MFB_KB_KEY_B, 'B');

if (keys[MFB_KB_KEY_0] &&
    !prev_keys[MFB_KB_KEY_0]) {
    if (rubiks_reset_callback) {
        rubiks_reset_callback();
}
}
      
        // Reset cube with 0
    if (keys[MFB_KB_KEY_0] && !prev_keys[MFB_KB_KEY_0]) {
        if (rubiks_reset_callback) {
            rubiks_reset_callback();
        }
    }

    // Save keyboard states
    for (int i = 0; i <= MFB_KB_KEY_LAST; ++i) {
        prev_keys[i] = keys[i];
    }

} // End of ui_bridge_input()
```



## 4. Cube animation

Face turns have a 0.25-second visual transition. During that transition the layer is drawn at an interpolated angle, and the exact quarter-turn is committed at the end.

**Key implementation detail:** The persistent puzzle state is not changed on every animation frame; it is updated only when the elapsed duration is reached.

### `rubiks_preview.h` — lines 11–21

```cpp
struct RubiksAnimation {
    bool active = false;
    char face = 'R';
    bool inverse = false;
    float elapsed = 0.0f;
    float duration = 0.25f;
};

inline RubiksAnimation& rubiks_animation() {
    static RubiksAnimation animation;
    return animation;
```



### `rubiks_preview.h` — lines 122–146

```cpp
inline void rubiks_turn(char face, bool inverse = false) {
    auto& animation = rubiks_animation();

    // Ignore additional moves during an animation
    if (animation.active)
        return;

    animation.active = true;
    animation.face = face;
    animation.inverse = inverse;
    animation.elapsed = 0.0f;
}

inline void rubiks_update(float delta_time) {
    auto& animation = rubiks_animation();

    if (!animation.active)
        return;

    animation.elapsed += delta_time;

    if (animation.elapsed >= animation.duration) {
        animation.active = false;
        rubiks_apply_turn(animation.face, animation.inverse);
    }
```


### `main.cpp` — lines 687–709

```cpp
      window);

  auto previousFrame = std::chrono::steady_clock::now();

  while (mfb_update_events(window) != MFB_STATE_EXIT) {
    // Input
    ui_bridge_input(ctx, window);

    auto currentFrame = std::chrono::steady_clock::now();

    float delta_time =
    std::chrono::duration<float>(
        currentFrame - previousFrame
    ).count();

    previousFrame = currentFrame;

// Avoid a huge jump after the application stalls
    delta_time = std::min(delta_time, 0.05f);

    rubiks_update(delta_time);
    rubiksEffectTime += delta_time;

```



### `rubiks_preview.h` — lines 575–610

```cpp
        const auto& animation = rubiks_animation();

    int animAxis = 0;
    int animLayer = 1;

    switch (animation.face) {
        case 'R': case 'r': animAxis = 0; animLayer =  1; break;
        case 'L': case 'l': animAxis = 0; animLayer = -1; break;
        case 'U': case 'u': animAxis = 1; animLayer =  1; break;
        case 'D': case 'd': animAxis = 1; animLayer = -1; break;
        case 'F': case 'f': animAxis = 2; animLayer =  1; break;
        case 'B': case 'b': animAxis = 2; animLayer = -1; break;
    }

    // Smoothstep easing: slow start, fast middle, slow finish
    float t = glm::clamp(
        animation.elapsed / animation.duration,
        0.0f, 1.0f
    );

    float eased = t * t * (3.0f - 2.0f * t);

    // Match the direction used by rubiks_apply_turn()
    int direction = -animLayer;
    if (animation.inverse)
        direction = -direction;

    float angle = glm::radians(90.0f) * direction * eased;

    glm::vec3 axisVector(0.0f);
    axisVector[animAxis] = 1.0f;

    glm::mat3 animatedRotation = glm::mat3(
        glm::rotate(glm::mat4(1.0f), angle, axisVector)
    );

```



### `rubiks_preview.h` — lines 658–670

```cpp
    for (const auto& p : rubiks_cubies()) {

        glm::vec3 center = glm::vec3(p.grid) * spacing;
        glm::mat3 orientation = p.orientation;

        // Only rotate cubies belonging to the moving layer
        if (animation.active && p.grid[animAxis] == animLayer) {
            center = animatedRotation * center;
            orientation = animatedRotation * orientation;
        }

        for (int f = 0; f < 6; ++f) {

```


## 5. Neon mode

Neon draws vivid sticker interiors, a depth-tested Gaussian glow, and crisp luminous outlines. Its pulse is based on elapsed time.

**Key implementation detail:** The glow uses a precomputed 9×9 Gaussian kernel in the uploaded header. Depth tests keep hidden edges from bleeding through.

### `main.cpp` — lines 30–36

```cpp
 // Rubik's Cube visual effects
static int rubiks_normal = 1;
static int rubiks_neon = 0;
static int rubiks_glass = 0;

static int use_phong_shading = 0; // Flat shading is much faster in software; UI can enable Phong
static int perspective_mode = 1;
```


### `main.cpp` — lines 767–801

```cpp
draw_rubiks_preview(
    view_matrix,
    perspective_matrix,
    final_matrix,
    rubiks_neon != 0,
    rubiksEffectTime,
    rubiks_glass != 0
);

    mu_begin(ctx);
    if (mu_begin_window(ctx, "Cube Modes", mu_rect(380, 20, 260, 210))) {
        int w[] = {-1};
        mu_layout_row(ctx, 1, w, 0);
        // These checkboxes act as an exclusive mode selector.
        int previousNormal = rubiks_normal;
        int previousNeon = rubiks_neon;
        int previousGlass = rubiks_glass;
        mu_checkbox(ctx, "Normal Cube", &rubiks_normal);
        mu_checkbox(ctx, "Neon Glow", &rubiks_neon);
        mu_checkbox(ctx, "Glass Mode", &rubiks_glass);
        if (rubiks_normal && !previousNormal) {
            rubiks_neon = 0;
            rubiks_glass = 0;
        } else if (rubiks_neon && !previousNeon) {
            rubiks_normal = 0;
            rubiks_glass = 0;
        } else if (rubiks_glass && !previousGlass) {
            rubiks_normal = 0;
            rubiks_neon = 0;
        }
        if (!rubiks_normal && !rubiks_neon && !rubiks_glass)
            rubiks_normal = 1;
        mu_end_window(ctx);
    }

```



### `rubiks_preview.h` — lines 149–205

```cpp
inline uint32_t rubiks_neon_color(uint32_t color, float pulse) {
    float r = float((color >> 16) & 255);
    float g = float((color >> 8) & 255);
    float b = float(color & 255);

    // Increase saturation and brightness
    float average = (r + g + b) / 3.0f;
    float intensity = 1.15f + 0.15f * pulse;

    r = std::clamp((r + (r - average) * 0.25f) * intensity, 0.0f, 255.0f);
    g = std::clamp((g + (g - average) * 0.25f) * intensity, 0.0f, 255.0f);
    b = std::clamp((b + (b - average) * 0.25f) * intensity, 0.0f, 255.0f);

    return MFB_RGB(
        static_cast<uint8_t>(r),
        static_cast<uint8_t>(g),
        static_cast<uint8_t>(b)
    );
}


inline uint32_t rubiks_dark_color(uint32_t color) {
    constexpr float brightness = 0.38f;

    uint8_t r = static_cast<uint8_t>(
        ((color >> 16) & 255) * brightness
    );
    uint8_t g = static_cast<uint8_t>(
        ((color >> 8) & 255) * brightness
    );
    uint8_t b = static_cast<uint8_t>(
        (color & 255) * brightness
    );

    return MFB_RGB(r, g, b);
}

inline uint32_t rubiks_blend_color(
    uint32_t background,
    uint32_t glow,
    float alpha
) {
    alpha = std::clamp(alpha, 0.0f, 1.0f);

    auto blend = [&](int shift) -> uint8_t {
        float bg = float((background >> shift) & 255);
        float fg = float((glow >> shift) & 255);

        return static_cast<uint8_t>(
            bg * (1.0f - alpha) + fg * alpha
        );
    };

    return MFB_RGB(
        blend(16),
        blend(8),
        blend(0)
```


### `rubiks_preview.h` — lines 212–222

```cpp
inline void draw_rubiks_preview(
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::mat4& final_matrix,
    bool neonMode = false,
    float time = 0.0f,
    bool glassMode = false
)
{
    // Neon glow pulses continuously
    float neonPulse = 0.5f + 0.5f * std::sin(time * 3.0f);
```


### `rubiks_preview.h` — lines 391–441

```cpp
    
    auto neonLine = [&](const glm::vec3& a,
                        const glm::vec3& b,
                        uint32_t color,
                        int thickness) {

        glm::vec4 clipA =
            projection * view * final_matrix * glm::vec4(a, 1.0f);
        glm::vec4 clipB =
            projection * view * final_matrix * glm::vec4(b, 1.0f);

        if (clipA.w <= 0.0f || clipB.w <= 0.0f)
            return;

        glm::vec3 ndcA = glm::vec3(clipA) / clipA.w;
        glm::vec3 ndcB = glm::vec3(clipB) / clipB.w;

        float x0 = (ndcA.x + 1.0f) * WIDTH * 0.5f;
        float y0 = (ndcA.y + 1.0f) * HEIGHT * 0.5f;
        float x1 = (ndcB.x + 1.0f) * WIDTH * 0.5f;
        float y1 = (ndcB.y + 1.0f) * HEIGHT * 0.5f;

        float dx = x1 - x0;
        float dy = y1 - y0;
        int steps = std::max(1, int(std::max(std::abs(dx), std::abs(dy))));

        for (int i = 0; i <= steps; ++i) {
            float t = float(i) / steps;
            int x = int(x0 + dx * t);
            int y = int(y0 + dy * t);
            float depth = clipA.w + (clipB.w - clipA.w) * t;

            for (int oy = -thickness; oy <= thickness; ++oy) {
                for (int ox = -thickness; ox <= thickness; ++ox) {
                    int px = x + ox;
                    int py = y + oy;

                    if (px < 0 || px >= WIDTH || py < 0 || py >= HEIGHT)
                        continue;

                    int index = py * WIDTH + px;

                    // Draw only if this edge is visible
                    if (depth <= z_buffer[index] + 3.0f) {
                        g_buffer[index] = color;
                    }
                }
            }
        }
    };

```


### `rubiks_preview.h` — lines 496–574

```cpp
    auto neonGlow = [&](const glm::vec3& a,
                        const glm::vec3& b,
                        uint32_t color) {

        glm::vec4 clipA =
            projection * view * final_matrix * glm::vec4(a, 1.0f);
        glm::vec4 clipB =
            projection * view * final_matrix * glm::vec4(b, 1.0f);

        if (clipA.w <= 0.0f || clipB.w <= 0.0f)
            return;

        glm::vec3 ndcA = glm::vec3(clipA) / clipA.w;
        glm::vec3 ndcB = glm::vec3(clipB) / clipB.w;

        float x0 = (ndcA.x + 1.0f) * WIDTH * 0.5f;
        float y0 = (ndcA.y + 1.0f) * HEIGHT * 0.5f;
        float x1 = (ndcB.x + 1.0f) * WIDTH * 0.5f;
        float y1 = (ndcB.y + 1.0f) * HEIGHT * 0.5f;

        float dx = x1 - x0;
        float dy = y1 - y0;

        int steps = std::max(
            1, int(std::max(std::abs(dx), std::abs(dy)))
        );

        constexpr int radius = 4;
        constexpr float sigma = 2.0f;
        static const auto glowKernel = [] {
            // Return std::array so the precomputed kernel has safe storage.
            std::array<std::array<float, 9>, 9> result{};
            for (int y = -4; y <= 4; ++y)
                for (int x = -4; x <= 4; ++x)
                    result[y + 4][x + 4] =
                        0.13f * std::exp(-float(x*x + y*y) / (2.0f * sigma * sigma));
            return result;
        }();

        for (int i = 0; i <= steps; ++i) {
            float t = float(i) / steps;

            float x = x0 + dx * t;
            float y = y0 + dy * t;

            float depth =
                clipA.w + (clipB.w - clipA.w) * t;

            int cx = int(x);
            int cy = int(y);

            for (int oy = -radius; oy <= radius; ++oy) {
                for (int ox = -radius; ox <= radius; ++ox) {

                    int px = cx + ox;
                    int py = cy + oy;

                    if (px < 0 || px >= WIDTH ||
                        py < 0 || py >= HEIGHT)
                        continue;

                    int index = py * WIDTH + px;

                    // Prevent hidden edges from glowing through the cube
                    if (depth > z_buffer[index] + 3.0f)
                        continue;

                    float alpha = glowKernel[oy + radius][ox + radius];

                    g_buffer[index] = rubiks_blend_color(
                        g_buffer[index],
                        color,
                        alpha
                    );
                }
            }
        }
    };

```


### `rubiks_preview.h` — lines 711–739

```cpp
            if (neonMode) {
                bool rotating =
                    animation.active &&
                    p.grid[animAxis] == animLayer;

                float pulse = rotating ? 1.0f : neonPulse;

                // Bright outline color
                uint32_t neonColor =
                    rubiks_neon_color(p.stickers[f], pulse);

                // Darker sticker interior
                uint32_t darkColor =
                    rubiks_dark_color(p.stickers[f]);

                quad(sticker, darkColor);

                
                for (int edge = 0; edge < 4; ++edge) {
                    neonEdges.push_back({
                        sticker[edge],
                        sticker[(edge + 1) % 4],
                        neonColor
                    });
                }

                

            }
```


### `rubiks_preview.h` — lines 784–800

```cpp
    // Draw visible neon edges after all cube faces
    
    // Neon rendering pass
    if (neonMode) {

        // First: soft glow
        for (const auto& edge : neonEdges) {
            neonGlow(edge.a, edge.b, edge.color);
        }

        // Second: sharp bright outlines
        for (const auto& edge : neonEdges) {
            neonLine(edge.a, edge.b, edge.color, 1);
        }
    }

}
```


## 6. Glass mode

Glass uses view-dependent specular and Fresnel terms, vertex-lit color and opacity, back-to-front alpha blending, and a highlight pass.

**Key implementation detail:** The glass color and opacity are computed at vertices and interpolated across covered pixels. The glass path is not the same as the per-pixel Phong checkbox path.

### `rubiks_preview.h` — lines 298–389

```cpp
    // Fast glass: evaluate lighting only at three vertices, then interpolate.
    // This preserves moving-light reflections while avoiding per-pixel pow/normalize.
    auto glassTriangle = [&](const glm::vec3& a,
                             const glm::vec3& b,
                             const glm::vec3& c,
                             uint32_t color,
                             float alpha) {
        const glm::vec3 local[3] = {a, b, c};
        glm::vec3 world[3];
        glm::vec4 clip[3];
        glm::vec2 screen[3];
        float depth[3];
        for (int i = 0; i < 3; ++i) {
            world[i] = glm::vec3(final_matrix * glm::vec4(local[i], 1.0f));
            clip[i] = projection * view * glm::vec4(world[i], 1.0f);
            if (clip[i].w <= 0.0f) return;
            glm::vec3 ndc = glm::vec3(clip[i]) / clip[i].w;
            screen[i] = glm::vec2((ndc.x + 1.0f) * WIDTH * 0.5f,
                                  (ndc.y + 1.0f) * HEIGHT * 0.5f);
            depth[i] = clip[i].w;
        }
        glm::vec3 N = glm::cross(world[1] - world[0], world[2] - world[0]);
        if (glm::dot(N, N) < 1e-10f) return;
        N = glm::normalize(N);
        glm::vec3 center = (world[0] + world[1] + world[2]) / 3.0f;
        if (glm::dot(N, camera.position - center) < 0.0f) N = -N;

        glm::vec3 tint(float((color >> 16) & 255) / 255.0f,
                       float((color >> 8) & 255) / 255.0f,
                       float(color & 255) / 255.0f);
        glm::vec3 lit[3];
        float opacity[3];
        for (int i = 0; i < 3; ++i) {
            glm::vec3 toView = camera.position - world[i];
            glm::vec3 toLight = light.position - world[i];
            glm::vec3 V = glm::normalize(toView);
            glm::vec3 L = glm::normalize(toLight);
            glm::vec3 H = glm::normalize(V + L);
            float ndv = std::clamp(glm::dot(N, V), 0.0f, 1.0f);
            float diffuse = std::max(glm::dot(N, L), 0.0f);
            float spec = std::pow(std::max(glm::dot(N, H), 0.0f), 90.0f);
            float fresnel = 0.06f + 0.94f * std::pow(1.0f - ndv, 5.0f);
            float transmission = 0.30f + 0.30f * diffuse;
            lit[i] = glm::clamp(
                tint * (light.ambient * 0.75f + light.diffuse * transmission) +
                light.specular * (0.85f * spec + 0.30f * fresnel),
                glm::vec3(0.0f), glm::vec3(1.0f));
            opacity[i] = std::clamp(alpha * (0.65f + 0.55f * fresnel) +
                                    0.18f * spec, 0.0f, 0.82f);
        }

        int minX = std::max(0, int(std::floor(std::min({screen[0].x, screen[1].x, screen[2].x}))));
        int maxX = std::min(WIDTH - 1, int(std::ceil(std::max({screen[0].x, screen[1].x, screen[2].x}))));
        int minY = std::max(0, int(std::floor(std::min({screen[0].y, screen[1].y, screen[2].y}))));
        int maxY = std::min(HEIGHT - 1, int(std::ceil(std::max({screen[0].y, screen[1].y, screen[2].y}))));
        if (minX > maxX || minY > maxY) return;

        auto edge = [](const glm::vec2& a, const glm::vec2& b, float x, float y) {
            return (x - a.x) * (b.y - a.y) - (y - a.y) * (b.x - a.x);
        };
        float area = edge(screen[0], screen[1], screen[2].x, screen[2].y);
        if (std::abs(area) < 1e-5f) return;
        float invArea = 1.0f / area;

        // Pixel loop: additions, multiplies, comparisons, and blending only.
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                float px = float(x) + 0.5f;
                float py = float(y) + 0.5f;
                float w0 = edge(screen[1], screen[2], px, py) * invArea;
                float w1 = edge(screen[2], screen[0], px, py) * invArea;
                float w2 = 1.0f - w0 - w1;
                if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f) continue;
                int index = y * WIDTH + x;
                float z = w0 * depth[0] + w1 * depth[1] + w2 * depth[2];
                if (z > z_buffer[index]) continue;
                glm::vec3 rgb = w0 * lit[0] + w1 * lit[1] + w2 * lit[2];
                float a = w0 * opacity[0] + w1 * opacity[1] + w2 * opacity[2];
                uint32_t litColor = MFB_RGB(uint8_t(rgb.r * 255.0f),
                                            uint8_t(rgb.g * 255.0f),
                                            uint8_t(rgb.b * 255.0f));
                g_buffer[index] = rubiks_blend_color(g_buffer[index], litColor, a);
            }
        }
    };

    auto glassQuad = [&](const glm::vec3 v[4],
                         uint32_t color,
                         float alpha) {
        glassTriangle(v[0], v[1], v[2], color, alpha);
        glassTriangle(v[0], v[2], v[3], color, alpha);
    };
```


### `rubiks_preview.h` — lines 382–394

```cpp
    };

    auto glassQuad = [&](const glm::vec3 v[4],
                         uint32_t color,
                         float alpha) {
        glassTriangle(v[0], v[1], v[2], color, alpha);
        glassTriangle(v[0], v[2], v[3], color, alpha);
    };

    
    auto neonLine = [&](const glm::vec3& a,
                        const glm::vec3& b,
                        uint32_t color,
```

### `rubiks_preview.h` — lines 443–494

```cpp
                          const glm::vec3& b,
                          uint32_t color,
                          float alpha) {

    glm::vec4 clipA =
        projection * view * final_matrix * glm::vec4(a, 1.0f);

    glm::vec4 clipB =
        projection * view * final_matrix * glm::vec4(b, 1.0f);

    if (clipA.w <= 0.0f || clipB.w <= 0.0f)
        return;

    glm::vec3 ndcA = glm::vec3(clipA) / clipA.w;
    glm::vec3 ndcB = glm::vec3(clipB) / clipB.w;

    float x0 = (ndcA.x + 1.0f) * WIDTH * 0.5f;
    float y0 = (ndcA.y + 1.0f) * HEIGHT * 0.5f;
    float x1 = (ndcB.x + 1.0f) * WIDTH * 0.5f;
    float y1 = (ndcB.y + 1.0f) * HEIGHT * 0.5f;

    float dx = x1 - x0;
    float dy = y1 - y0;

    int steps = std::max(
        1,
        int(std::max(std::abs(dx), std::abs(dy)))
    );

    for (int i = 0; i <= steps; ++i) {
        float t = float(i) / steps;

        int x = int(x0 + dx * t);
        int y = int(y0 + dy * t);

        if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
            continue;

        int index = y * WIDTH + x;

        float depth =
            clipA.w + (clipB.w - clipA.w) * t;

        if (depth <= z_buffer[index] + 3.0f) {
            g_buffer[index] = rubiks_blend_color(
                g_buffer[index],
                color,
                alpha
            );
        }
    }
};
```


### `rubiks_preview.h` — lines 612–657

```cpp
    struct NeonEdge {
        glm::vec3 a;
        glm::vec3 b;
        uint32_t color;
    };

    std::vector<NeonEdge> neonEdges;
neonEdges.reserve(220);
    struct GlassFace {
    glm::vec3 vertices[4];
    uint32_t color;
    float alpha;
    float depth;
    bool sticker;
};

std::vector<GlassFace> glassFaces;
glassFaces.reserve(180);

auto collectGlassFace = [&](const glm::vec3 v[4],
                            uint32_t color,
                            float alpha,
                            bool sticker) {
    GlassFace face{};

    glm::vec3 center(0.0f);

    for (int k = 0; k < 4; ++k) {
        face.vertices[k] = v[k];
        center += v[k];
    }

    center *= 0.25f;

    glm::vec4 cameraPoint =
        view * final_matrix * glm::vec4(center, 1.0f);

    face.depth = -cameraPoint.z;
    face.color = color;
    face.alpha = alpha;
    face.sticker = sticker;

    glassFaces.push_back(face);
};


```


### `rubiks_preview.h` — lines 671–710

```cpp
    // Glass Mode: skip faces pointing away from the camera
    if (glassMode && !neonMode) {
        glm::vec3 worldNormal = orientation * normals[f];

        glm::vec3 faceCenter =
            center + worldNormal * halfSize;

        glm::vec3 cameraCenter = glm::vec3(
            view * final_matrix * glm::vec4(faceCenter, 1.0f)
        );

        glm::vec3 cameraNormal = glm::normalize(
            glm::mat3(view * final_matrix) * worldNormal
        );

        glm::vec3 toCamera = glm::normalize(-cameraCenter);

        if (glm::dot(cameraNormal, toCamera) <= 0.0f)
            continue;
    }

    glm::vec3 body[4];
    for (int k = 0; k < 4; ++k)
        body[k] = center + orientation * (corners[f][k] * halfSize);
            
            if (glassMode && !neonMode) {
                collectGlassFace(body,MFB_RGB(120, 190, 225),0.23f,false);
            } else {
                    quad(body, MFB_RGB(20, 22, 28));
            }

            if (p.stickers[f] == 0) continue;
            glm::vec3 sticker[4];
            for (int k=0; k<4; ++k) {
                glm::vec3 v = corners[f][k];
                sticker[k] = center + orientation *
                    ((v - normals[f])*stickerHalf + normals[f]*stickerOffset);
            }
            
            
```


### `rubiks_preview.h` — lines 740–783

```cpp
            
            else if (glassMode) {
                collectGlassFace(
                sticker,
                p.stickers[f],
                0.48f,
                true
            );
            }
            else {
                quad(sticker, p.stickers[f]);
            }


        }
    }
    // Glass Mode: render transparent surfaces back to front
if (glassMode && !neonMode) {

    std::sort(
        glassFaces.begin(),
        glassFaces.end(),
        [](const GlassFace& a, const GlassFace& b) {
            return a.depth > b.depth;
        }
    );

    // First pass: draw all transparent surfaces once.
    for (const auto& face : glassFaces) {
        glassQuad(face.vertices, face.color, face.alpha);
    }

    // Second pass: draw highlights once, after the surfaces.
    for (const auto& face : glassFaces) {
        if (!face.sticker) continue;
        uint32_t reflectionColor = MFB_RGB(225, 245, 255);
        for (int edge = 0; edge < 4; ++edge) {
            glassHighlight(face.vertices[edge],
                           face.vertices[(edge + 1) % 4],
                           reflectionColor, 0.25f);
        }
    }
}

```


## 7. Connecting the cube to lighting

Sticker colors become temporary material values, and the cube uses the application's light, camera, Flat shading, and Phong shading functions.

**Key implementation detail:** The material is temporarily recolored from each sticker's RGB, then restored. Flat and Phong share light parameters but differ in how often lighting is evaluated.

### `main.cpp` — lines 64–81

```cpp
struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct Material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
};

struct Camera {//camera structure
    glm::vec3 position;
    glm::vec3 rotation;
};
```


### `main.cpp` — lines 103–121

```cpp
static PointLight light = {
    glm::vec3(0.0f, 0.0f, 1000.0f),
    glm::vec3(1.0f, 1.0f, 1.0f),
    glm::vec3(1.0f, 1.0f, 1.0f),
    glm::vec3(1.0f, 1.0f, 1.0f)
};

static Material material = {
    glm::vec3(0.8f, 0.5f, 0.3f),
    glm::vec3(1.0f, 1.0f, 1.0f),
    glm::vec3(1.0f, 1.0f, 1.0f), 
    32.0f
};

// HW3 Part 2: Camera
static Camera camera = {
    glm::vec3(0.0f, 0.0f, 1500.0f),
    glm::vec3(0.0f, 0.0f, 0.0f)
};
```


### `main.cpp` — lines 418–503

```cpp
uint32_t calculate_ambient_color() {

    glm::vec3 ambient_color = light.ambient * material.ambient;
    ambient_color = glm::clamp(ambient_color,glm::vec3(0.0f),glm::vec3(1.0f));

    return MFB_RGB((uint8_t)(ambient_color.r * 255.0f),(uint8_t)(ambient_color.g * 255.0f),(uint8_t)(ambient_color.b * 255.0f));
}
glm::vec3 calculate_reflection(glm::vec3 incoming,glm::vec3 normal) {
    return incoming - 2.0f * glm::dot(incoming, normal) * normal;//reflection formula
}

uint32_t calculate_phong_lighting(glm::vec3 position,glm::vec3 normal) {
    normal = glm::normalize(normal);
    glm::vec3 ambient = light.ambient * material.ambient;

    // Direction toward the light
    glm::vec3 to_light = light.position - position;
    if (glm::length(to_light) < 0.000001f) {
        return calculate_ambient_color();
    }

    glm::vec3 light_direction = glm::normalize(to_light);

    // Diffuse
    float diffuse_strength = std::max(glm::dot(normal, light_direction),0.0f);
    glm::vec3 diffuse = light.diffuse * material.diffuse * diffuse_strength;

    // Specular
    glm::vec3 incoming = -light_direction;

    glm::vec3 reflection = glm::normalize(calculate_reflection(incoming, normal));

    glm::vec3 to_camera = camera.position - position;
    glm::vec3 specular(0.0f);

    if (diffuse_strength > 0.0f && glm::length(to_camera) > 0.000001f) {

        glm::vec3 view_direction = glm::normalize(to_camera);

        float specular_strength = pow(std::max(glm::dot(reflection, view_direction),0.0f),material.shininess);

        specular = light.specular * material.specular * specular_strength;
    }

    // Final color
    glm::vec3 final_color = ambient + diffuse + specular;

    final_color = glm::clamp(final_color,glm::vec3(0.0f),glm::vec3(1.0f));

    return MFB_RGB((uint8_t)(final_color.r * 255.0f),(uint8_t)(final_color.g * 255.0f),(uint8_t)(final_color.b * 255.0f));
}

uint32_t calculate_flat_shading(glm::vec3 v0,glm::vec3 v1,glm::vec3 v2) {
    glm::vec3 ambient = light.ambient * material.ambient;   // Ambient lighting
    glm::vec3 center = (v0 + v1 + v2) / 3.0f;// Calculate triangle center

    // Calculate face normal
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;

    if (glm::length(glm::cross(edge1, edge2)) < 0.000001f) {
        return calculate_ambient_color();
    }
    glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
    glm::vec3 light_direction = glm::normalize(light.position - center);// Direction from triangle center to light
    if (glm::length(light.position - center) < 0.000001f) {
        return calculate_ambient_color();
    }
    // Lambert's cosine law
    float diffuse_strength = std::max(glm::dot(normal, light_direction),0.0f);// Diffuse strength
    glm::vec3 diffuse = light.diffuse * material.diffuse * diffuse_strength;// Diffuse lighting
    glm::vec3 incoming = -light_direction;
    glm::vec3 reflection = glm::normalize(calculate_reflection(incoming, normal));
    glm::vec3 view_direction = glm::normalize(camera.position - center);// Direction from surface toward camera
    float specular_strength = 0.0f;
    if (diffuse_strength > 0.0f) {
            specular_strength = pow(std::max(glm::dot(reflection, view_direction), 0.0f),material.shininess);
    }
    glm::vec3 specular = light.specular * material.specular * specular_strength;
    glm::vec3 final_color = ambient + diffuse + specular;// Combine ambient and diffuse

    // Keep RGB values between 0 and 1
    final_color = glm::clamp(final_color,glm::vec3(0.0f),glm::vec3(1.0f));

    return MFB_RGB((uint8_t)(final_color.r * 255.0f),(uint8_t)(final_color.g * 255.0f),(uint8_t)(final_color.b * 255.0f));
}
```


### `main.cpp` — lines 505–557

```cpp
void draw_phong_triangle(int x0, int y0, float z0,int x1, int y1, float z1,int x2, int y2, float z2,glm::vec3 p0, glm::vec3 p1, glm::vec3 p2,glm::vec3 n0, glm::vec3 n1, glm::vec3 n2) {
    // Bounding rectangle
    int x_min = std::max(0, std::min({x0, x1, x2}));
    int x_max = std::min(WIDTH - 1, std::max({x0, x1, x2}));

    int y_min = std::max(0, std::min({y0, y1, y2}));
    int y_max = std::min(HEIGHT - 1, std::max({y0, y1, y2}));

    float denominator =(float)((y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2));

    if (denominator == 0.0f) {
        return;
    }

    for (int y = y_min; y <= y_max; y++) {
        for (int x = x_min; x <= x_max; x++) {

            // Barycentric coordinates
            float alpha =((y1 - y2) * (x - x2) +(x2 - x1) * (y - y2)) / denominator;
            float beta =((y2 - y0) * (x - x2) + (x0 - x2) * (y - y2)) / denominator;
            float gamma = 1.0f - alpha - beta;

            if (alpha >= 0.0f && beta >= 0.0f && gamma >= 0.0f) {

                // Interpolate depth
                float z =alpha * z0 +beta * z1 +gamma * z2;

                int index = y * WIDTH + x;

                if (z < z_buffer[index]) {

                    // Interpolate world-space position
                    glm::vec3 pixel_position = alpha * p0 + beta * p1 + gamma * p2;

                    // Interpolate vertex normals
                    glm::vec3 pixel_normal = alpha * n0 + beta * n1 + gamma * n2;

                    // Normalize interpolated normal
                    if (glm::length(pixel_normal) < 0.000001f) {
                        continue;
                    }

                    pixel_normal = glm::normalize(pixel_normal);

                    // Calculate lighting for this pixel
                    uint32_t color = calculate_phong_lighting(pixel_position,pixel_normal);

                    z_buffer[index] = z;
                    g_buffer[index] = color;
                }
            }
        }
    }
```


### `rubiks_preview.h` — lines 241–296

```cpp
    // Reuse the application's existing Flat/Phong lighting functions.
    // Each sticker supplies its own material color.
    auto triangle = [&](const glm::vec3& a, const glm::vec3& b,
                        const glm::vec3& c, uint32_t baseColor) {
        glm::vec3 world[3] = {
            glm::vec3(final_matrix * glm::vec4(a, 1.0f)),
            glm::vec3(final_matrix * glm::vec4(b, 1.0f)),
            glm::vec3(final_matrix * glm::vec4(c, 1.0f))
        };
        glm::vec4 clip[3] = {
            projection * view * glm::vec4(world[0], 1.0f),
            projection * view * glm::vec4(world[1], 1.0f),
            projection * view * glm::vec4(world[2], 1.0f)
        };
        for (const auto& p : clip) if (p.w <= 0.0f) return;

        int sx[3], sy[3];
        float depth[3];
        for (int i = 0; i < 3; ++i) {
            glm::vec3 ndc = glm::vec3(clip[i]) / clip[i].w;
            sx[i] = int((ndc.x + 1.0f) * WIDTH * 0.5f);
            sy[i] = int((ndc.y + 1.0f) * HEIGHT * 0.5f);
            depth[i] = clip[i].w;
        }

        glm::vec3 cross = glm::cross(world[1] - world[0], world[2] - world[0]);
        if (glm::dot(cross, cross) < 1e-10f) return;
        glm::vec3 normal = glm::normalize(cross);

        Material savedMaterial = material;
        glm::vec3 base(float((baseColor >> 16) & 255) / 255.0f,
                       float((baseColor >> 8) & 255) / 255.0f,
                       float(baseColor & 255) / 255.0f);
        material.ambient = base * 0.45f;
        material.diffuse = base;
        material.specular = glm::vec3(0.65f);

        if (use_phong_shading) {
            draw_phong_triangle(sx[0], sy[0], depth[0],
                                sx[1], sy[1], depth[1],
                                sx[2], sy[2], depth[2],
                                world[0], world[1], world[2],
                                normal, normal, normal);
        } else {
            uint32_t shaded = calculate_flat_shading(world[0], world[1], world[2]);
            draw_filled_triangle(sx[0], sy[0], depth[0],
                                 sx[1], sy[1], depth[1],
                                 sx[2], sy[2], depth[2], shaded);
        }
        material = savedMaterial;
    };

    auto quad = [&](const glm::vec3 v[4], uint32_t color) {
        triangle(v[0], v[1], v[2], color);
        triangle(v[0], v[2], v[3], color);
    };
```


### `main.cpp` — lines 826–865

```cpp
    if (mu_begin_window(ctx, "Lighting", mu_rect(1200, 20, 350, 900))) {
        int w[] = {-1};
        mu_layout_row(ctx, 1, w, 0);
        mu_checkbox(ctx, "Phong Shading", &use_phong_shading);
        mu_label(ctx, "LIGHT POSITION");
        mu_label(ctx, "Light X");
        mu_slider(ctx, &light.position.x, -2000.0f, 2000.0f);
        mu_label(ctx, "Light Y");
        mu_slider(ctx, &light.position.y, -2000.0f, 2000.0f);
        mu_label(ctx, "Light Z");
        mu_slider(ctx, &light.position.z, -2000.0f, 2000.0f);

        mu_label(ctx, "AMBIENT LIGHT");
        mu_label(ctx, "Ambient Red");
        mu_slider(ctx, &light.ambient.r, 0.0f, 1.0f);
        mu_label(ctx, "Ambient Green");
        mu_slider(ctx, &light.ambient.g, 0.0f, 1.0f);
        mu_label(ctx, "Ambient Blue");
        mu_slider(ctx, &light.ambient.b, 0.0f, 1.0f);

        mu_label(ctx, "DIFFUSE LIGHT");
        mu_label(ctx, "Diffuse Red");
        mu_slider(ctx, &light.diffuse.r, 0.0f, 1.0f);
        mu_label(ctx, "Diffuse Green");
        mu_slider(ctx, &light.diffuse.g, 0.0f, 1.0f);
        mu_label(ctx, "Diffuse Blue");
        mu_slider(ctx, &light.diffuse.b, 0.0f, 1.0f);

        mu_label(ctx, "SPECULAR LIGHT");
        mu_label(ctx, "Specular Red");
        mu_slider(ctx, &light.specular.r, 0.0f, 1.0f);
        mu_label(ctx, "Specular Green");
        mu_slider(ctx, &light.specular.g, 0.0f, 1.0f);
        mu_label(ctx, "Specular Blue");
        mu_slider(ctx, &light.specular.b, 0.0f, 1.0f);

        mu_label(ctx, "Material Shininess");
        mu_slider(ctx, &material.shininess, 1.0f, 128.0f);
        mu_end_window(ctx);
    }
```


## 8. Flat versus per-pixel Phong performance

Flat shading computes one lighting result per triangle; Phong computes lighting for each covered visible pixel. Both still rasterize triangles and depth-test pixels.

**Key implementation detail:** This is an algorithmic explanation, not an FPS benchmark. The actual speed depends on build flags, screen resolution, visible geometry, and CPU.

### `main.cpp` — lines 24–28

```cpp
#define WIDTH 1600
#define HEIGHT 1200

static uint32_t g_buffer[WIDTH * HEIGHT];
static float z_buffer[WIDTH * HEIGHT];
```


### `main.cpp` — lines 329–377

```cpp
// fill triangle using barycentric coordinates
void draw_filled_triangle(int x0, int y0, float z0, int x1, int y1,float z1, int x2, int y2,float z2, uint32_t color) {

    // find the bounding rectangle
    int x_min = std::min(x0, std::min(x1, x2));
    int x_max = std::max(x0, std::max(x1, x2));

    int y_min = std::min(y0, std::min(y1, y2));
    int y_max = std::max(y0, std::max(y1, y2));

    // keep coordinates inside the screen
    x_min = std::max(0, x_min);
    x_max = std::min(WIDTH - 1, x_max);

    y_min = std::max(0, y_min);
    y_max = std::min(HEIGHT - 1, y_max);

    // calculate denominator
    float denominator = (float)((y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2));

    // avoid division by zero
    if (denominator == 0.0f) {
        return;
    }

    // check every pixel inside the bounding rectangle
    for (int y = y_min; y <= y_max; y++) {
        for (int x = x_min; x <= x_max; x++) {

            // calculate barycentric coordinates
            float alpha = ((y1 - y2) * (x - x2) + (x2 - x1) * (y - y2)) / denominator;
            float beta = ((y2 - y0) * (x - x2) + (x0 - x2) * (y - y2)) / denominator;
            float gamma = 1.0f - alpha - beta;

            // check whether pixel is inside triangle
            if (alpha >= 0.0f && beta >= 0.0f && gamma >= 0.0f) {
                // Interpolate pixel depth
                float z = alpha * z0 + beta * z1 + gamma * z2;
                int index = y * WIDTH + x;
                // Depth test
                if (z < z_buffer[index]) {
                    z_buffer[index] = z;
                    g_buffer[index] = color;
            }
            }
        }
    }
}

```


### `main.cpp` — lines 470–503

```cpp
uint32_t calculate_flat_shading(glm::vec3 v0,glm::vec3 v1,glm::vec3 v2) {
    glm::vec3 ambient = light.ambient * material.ambient;   // Ambient lighting
    glm::vec3 center = (v0 + v1 + v2) / 3.0f;// Calculate triangle center

    // Calculate face normal
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;

    if (glm::length(glm::cross(edge1, edge2)) < 0.000001f) {
        return calculate_ambient_color();
    }
    glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
    glm::vec3 light_direction = glm::normalize(light.position - center);// Direction from triangle center to light
    if (glm::length(light.position - center) < 0.000001f) {
        return calculate_ambient_color();
    }
    // Lambert's cosine law
    float diffuse_strength = std::max(glm::dot(normal, light_direction),0.0f);// Diffuse strength
    glm::vec3 diffuse = light.diffuse * material.diffuse * diffuse_strength;// Diffuse lighting
    glm::vec3 incoming = -light_direction;
    glm::vec3 reflection = glm::normalize(calculate_reflection(incoming, normal));
    glm::vec3 view_direction = glm::normalize(camera.position - center);// Direction from surface toward camera
    float specular_strength = 0.0f;
    if (diffuse_strength > 0.0f) {
            specular_strength = pow(std::max(glm::dot(reflection, view_direction), 0.0f),material.shininess);
    }
    glm::vec3 specular = light.specular * material.specular * specular_strength;
    glm::vec3 final_color = ambient + diffuse + specular;// Combine ambient and diffuse

    // Keep RGB values between 0 and 1
    final_color = glm::clamp(final_color,glm::vec3(0.0f),glm::vec3(1.0f));

    return MFB_RGB((uint8_t)(final_color.r * 255.0f),(uint8_t)(final_color.g * 255.0f),(uint8_t)(final_color.b * 255.0f));
}
```



### `main.cpp` — lines 505–557

```cpp
void draw_phong_triangle(int x0, int y0, float z0,int x1, int y1, float z1,int x2, int y2, float z2,glm::vec3 p0, glm::vec3 p1, glm::vec3 p2,glm::vec3 n0, glm::vec3 n1, glm::vec3 n2) {
    // Bounding rectangle
    int x_min = std::max(0, std::min({x0, x1, x2}));
    int x_max = std::min(WIDTH - 1, std::max({x0, x1, x2}));

    int y_min = std::max(0, std::min({y0, y1, y2}));
    int y_max = std::min(HEIGHT - 1, std::max({y0, y1, y2}));

    float denominator =(float)((y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2));

    if (denominator == 0.0f) {
        return;
    }

    for (int y = y_min; y <= y_max; y++) {
        for (int x = x_min; x <= x_max; x++) {

            // Barycentric coordinates
            float alpha =((y1 - y2) * (x - x2) +(x2 - x1) * (y - y2)) / denominator;
            float beta =((y2 - y0) * (x - x2) + (x0 - x2) * (y - y2)) / denominator;
            float gamma = 1.0f - alpha - beta;

            if (alpha >= 0.0f && beta >= 0.0f && gamma >= 0.0f) {

                // Interpolate depth
                float z =alpha * z0 +beta * z1 +gamma * z2;

                int index = y * WIDTH + x;

                if (z < z_buffer[index]) {

                    // Interpolate world-space position
                    glm::vec3 pixel_position = alpha * p0 + beta * p1 + gamma * p2;

                    // Interpolate vertex normals
                    glm::vec3 pixel_normal = alpha * n0 + beta * n1 + gamma * n2;

                    // Normalize interpolated normal
                    if (glm::length(pixel_normal) < 0.000001f) {
                        continue;
                    }

                    pixel_normal = glm::normalize(pixel_normal);

                    // Calculate lighting for this pixel
                    uint32_t color = calculate_phong_lighting(pixel_position,pixel_normal);

                    z_buffer[index] = z;
                    g_buffer[index] = color;
                }
            }
        }
    }
```



### `main.cpp` — lines 710–713

```cpp
       // Fast, uniform charcoal background.
    std::fill(g_buffer, g_buffer + WIDTH * HEIGHT, MFB_RGB(32, 35, 42));
    // Clear Z-buffer once per frame
    std::fill(z_buffer,z_buffer + WIDTH * HEIGHT,std::numeric_limits<float>::infinity());
```



### `rubiks_preview.h` — lines 270–290

```cpp
        Material savedMaterial = material;
        glm::vec3 base(float((baseColor >> 16) & 255) / 255.0f,
                       float((baseColor >> 8) & 255) / 255.0f,
                       float(baseColor & 255) / 255.0f);
        material.ambient = base * 0.45f;
        material.diffuse = base;
        material.specular = glm::vec3(0.65f);

        if (use_phong_shading) {
            draw_phong_triangle(sx[0], sy[0], depth[0],
                                sx[1], sy[1], depth[1],
                                sx[2], sy[2], depth[2],
                                world[0], world[1], world[2],
                                normal, normal, normal);
        } else {
            uint32_t shaded = calculate_flat_shading(world[0], world[1], world[2]);
            draw_filled_triangle(sx[0], sy[0], depth[0],
                                 sx[1], sy[1], depth[1],
                                 sx[2], sy[2], depth[2], shaded);
        }
        material = savedMaterial;
```


### `main.cpp` — lines 866–883

```cpp
    mu_end(ctx);

    // 4. UI Rendering
    renderer.render(ctx, g_buffer);

    // 5. Display
    mfb_update_state state = mfb_update_ex(window, g_buffer, WIDTH, HEIGHT);
    if (state < 0)
      break;

    // Cap FPS (optional, minifb has built-in sync)
    mfb_wait_sync(window);
  }

  mfb_close(window);
  free(ctx);
  return 0;
}
```


## Conclusion

The Rubik's Cube separates puzzle-state rotations from whole-model transformations. The face-turn animation uses temporary interpolated matrices before committing exact quarter-turn state changes. Neon emphasizes edges and glow; Glass blends lit transparent faces. Flat shading is the responsive default for the CPU rasterizer, while per-pixel Phong is optional and more computationally expensive.

