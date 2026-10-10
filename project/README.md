# Computer Graphics 2026 - Rubik cube project

**Student Information**
* **Name:** [Larysa Nechay]
* **Student ID:** [337575559]

# Project report:

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

Next I moved to implement the rotation mechanics of the cube so we could rotate it to play.

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


This code connects the main program to the UI bridge. It registers the functions used to rotate and reset the Rubik's Cube, then passes pointers to the local and world transformation variables. As a result, keyboard and mouse input can directly update the values that control the cube's appearance.

### `rubiks_preview.h` — lines 85–119

```
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


This code handles the actual 90-degree face rotations. It first chooses the correct rotation axis and layer, then updates the position and orientation of every cubie in that layer. The results are rounded to avoid floating-point errors. The second function connects the standard Rubik's Cube moves (R, L, U, D, F and B) to the correct axis, layer and direction, including inverse turns.

### `rubiks_preview.h` — lines 122–146

```
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


These functions control when a face turn starts and finishes. `rubiks_turn()` records the requested move and prevents another turn from starting during an active animation. `rubiks_update()` advances the timer each frame and applies the final 90-degree rotation only after the animation is complete.

### `ui_bridge.h` — lines 14–48

```
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


This code defines the possible coordinate frames, transformation types and axes. It also declares callback types for Rubik's Cube turns and resets, then provides a binding function that connects those callbacks to the cube functions in `main.cpp`. This keeps the input system separate from the puzzle logic.

### `ui_bridge.h` — lines 520–562

```
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


This code connects the Rubik's Cube face moves to the keyboard. Pressing R, L, U, D, F or B starts the corresponding turn, while holding Shift reverses it. Pressing 0 resets the cube. Previous key states prevent a held key from repeatedly triggering moves; the reset check appears twice in this excerpt, which is redundant.

### `rubiks_preview.h` — lines 11–21

```
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


This structure stores everything needed for a face-turn animation: whether it is active, which face is moving, whether the turn is reversed, how much time has passed and its duration. The accessor returns one shared animation object so the update and drawing functions use the same state.

### `rubiks_preview.h` — lines 122–146

```
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


These functions control when a face turn starts and finishes. `rubiks_turn()` records the requested move and prevents another turn from starting during an active animation. `rubiks_update()` advances the timer each frame and applies the final 90-degree rotation only after the animation is complete.

### `main.cpp` — lines 687–709

```
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


This is the beginning of the application's frame loop. It reads input, measures the time since the previous frame and limits unusually large time steps. The measured time is then used to update the cube's rotation animation and the timer for visual effects such as the neon pulse.

### `rubiks_preview.h` — lines 575–610

```
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


This code prepares the temporary rotation used to animate a face turn. It identifies the moving axis and layer, calculates how far the animation has progressed and applies smoothstep easing so the turn starts and ends gently. It then creates a rotation matrix for the current partial angle, without permanently changing the puzzle state yet.

### `rubiks_preview.h` — lines 658–670

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

```


This code loops through the cubies and calculates each one's drawing position and orientation. If a cubie belongs to the layer currently being animated, it receives the temporary rotation matrix. The other cubies stay in place, making the selected face appear to turn smoothly.

### `main.cpp` — lines 30–36

```
 // Rubik's Cube visual effects
static int rubiks_normal = 1;
static int rubiks_neon = 0;
static int rubiks_glass = 0;

static int use_phong_shading = 0; // Flat shading is much faster in software; UI can enable Phong
static int perspective_mode = 1;
```


These variables store the selected cube appearance and shading mode. Normal mode is enabled by default, while Neon, Glass and per-pixel Phong shading start disabled. The flags are later used to choose which rendering effects the application should draw.

### `main.cpp` — lines 767–801

```
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


This code draws the cube using the current view, projection, transformation and visual-effect settings. It also creates three MicroUI checkboxes for Normal, Neon and Glass modes. Selecting one mode switches the others off, ensuring that only one cube appearance is active at a time.

### `rubiks_preview.h` — lines 149–205

```
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


These helper functions prepare colors for the visual effects. One brightens and saturates the sticker colors for Neon Mode, another darkens their interiors to make the outlines stand out, and a third blends two colors using an alpha value. The blending function is reused to create transparent and glowing effects.

### `rubiks_preview.h` — lines 212–222

```
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


This code defines the cube-rendering function and receives the matrices and settings needed to draw it. It also calculates a sine-based pulse from elapsed time, allowing the neon effect to become brighter and dimmer continuously.

### `rubiks_preview.h` — lines 391–441

```
    
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


This function draws a sharp neon line between two 3D points. It projects the endpoints onto the screen, steps along the resulting line and draws pixels with the chosen thickness. Screen-boundary and depth checks prevent invalid pixels and help keep hidden edges from appearing through the cube.

### `rubiks_preview.h` — lines 496–574

```
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


This function creates the soft glow surrounding each neon edge. It projects the edge onto the screen and blends color around each line point using a precomputed 9×9 Gaussian kernel. The kernel makes the glow strongest near the line and weaker farther away, while depth checks stop hidden edges from glowing through visible surfaces.

### `rubiks_preview.h` — lines 711–739

```
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


This code chooses how stickers are drawn in Neon Mode. It creates a bright outline color, uses a darker color for the sticker interior and stores the four sticker edges for later drawing. Cubies in the currently rotating layer receive maximum pulse intensity, making the moving face stand out.

### `rubiks_preview.h` — lines 784–800

```
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


This is the final Neon Mode drawing pass. It first draws the soft glow around every collected edge, then draws a sharp bright line on top. Using two passes gives the cube both a blurred halo and clearly visible neon outlines.

### `rubiks_preview.h` — lines 298–389

```
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


This function renders a transparent glass triangle. It transforms its vertices into screen coordinates, calculates the surface normal and evaluates lighting at the three vertices, including specular highlights and view-dependent Fresnel reflection. It then interpolates color, depth and opacity across the triangle's pixels and blends them with the framebuffer, avoiding expensive lighting calculations for every pixel.

### `rubiks_preview.h` — lines 382–394

```
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


This small helper draws a four-sided glass face by splitting it into two triangles. Both triangles use the glass-rendering function, allowing the same transparency and lighting effects to cover the entire face.

### `rubiks_preview.h` — lines 443–494

```
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


This code draws bright highlights along glass edges. It projects each 3D edge into screen coordinates, walks along its pixels and blends a light-colored highlight where the edge is visible. This helps the transparent cube retain clear outlines and look more reflective.

### `rubiks_preview.h` — lines 612–657

```
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


This code defines the data structures used to collect neon edges and transparent glass faces. For glass faces, it stores the geometry, color, opacity and depth information needed for later drawing. Collecting the faces first allows them to be sorted and rendered in the correct order for transparency.

### `rubiks_preview.h` — lines 671–710

```
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


This code builds the visible body faces and colored stickers for each cubie. In Glass Mode it skips faces pointing away from the camera and collects visible body faces for transparent rendering; otherwise it draws the dark cubie bodies normally. Sticker geometry is calculated separately and positioned slightly above each cubie surface.

### `rubiks_preview.h` — lines 740–783

```
            
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


This code handles the final collection and drawing of Glass Mode surfaces. It stores the transparent faces, sorts them from back to front and blends them in that order so overlapping glass looks more natural. It then draws the sticker-edge highlights over the transparent surfaces.


### `rubiks_preview.h` — lines 241–296

```
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


This code connects each cube triangle to the application's rendering and lighting system. It transforms the vertices, calculates screen positions and prepares a material color based on the sticker. Depending on the shading setting, it draws the triangle with either flat shading or per-pixel Phong shading, then restores the previous material.

### `main.cpp` — lines 826–865

```
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


This code creates the Lighting panel in MicroUI. Its controls let us turn Phong shading on or off and adjust the light's position, ambient/diffuse/specular colors and material shininess. These values are used directly by the lighting calculations on the following frames.

### `main.cpp` — lines 710–713

```cpp
       // Fast, uniform charcoal background.
    std::fill(g_buffer, g_buffer + WIDTH * HEIGHT, MFB_RGB(32, 35, 42));
    // Clear Z-buffer once per frame
    std::fill(z_buffer,z_buffer + WIDTH * HEIGHT,std::numeric_limits<float>::infinity());
```


At the start of each frame, this code fills the color buffer with a uniform charcoal background and resets every depth value to infinity. The simple background avoids unnecessary effects, while the depth reset prepares the renderer to determine which cube surfaces are visible in the new frame.

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


This code chooses between the two shading methods when drawing a cube triangle. It first assigns material colors from the sticker. If Phong is enabled, it calculates lighting across the triangle's pixels; otherwise, it computes one flat-shaded color and fills the triangle with it. Finally, it restores the original material.

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


This code finishes each frame by rendering the MicroUI interface into the framebuffer and displaying the result through MiniFB. It waits for synchronization, continues until the window closes, then releases resources. Keeping the simpler flat-shading path active helps this loop remain responsive.

## Conclusion

The Rubik's Cube separates puzzle-state rotations from whole-model transformations. The face-turn animation uses temporary interpolated matrices before committing exact quarter-turn state changes. Neon emphasizes edges and glow; Glass blends lit transparent faces. Flat shading is the responsive default for the CPU rasterizer, while per-pixel Phong is optional and more computationally expensive.
