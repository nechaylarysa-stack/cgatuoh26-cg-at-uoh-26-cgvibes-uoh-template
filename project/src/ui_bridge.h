#pragma once
#include "MiniFB.h"
#include "rubiks_preview.h"
extern "C" {
#include "microui.h"
}

static char g_pending_text[32] = {};
static int g_pending_text_len = 0;

// ============================================================
// Part 6: Transformation input
// ============================================================

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
}


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

// ========================================================
// RUBIK'S CUBE KEYBOARD CONTROLS
// ========================================================

bool reverse =
    keys[MFB_KB_KEY_LEFT_SHIFT] ||
    keys[MFB_KB_KEY_RIGHT_SHIFT];

auto cube_key = [&](int key, char face) {
    if (keys[key] && !prev_keys[key]) {
        rubiks_turn(face, reverse);
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
    rubiks_reset();
}
    // ========================================================
    // Save keyboard states for next frame
    // ========================================================

    for (int i = 0; i <= MFB_KB_KEY_LAST; i++) {
        prev_keys[i] = keys[i];
    }
}
