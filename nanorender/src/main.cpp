#include "MiniFB.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <glm/glm.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

extern "C" {
#include "microui.h"
}
#include "ui_bridge.h"
#include "ui_renderer.h"

#define WIDTH 1600
#define HEIGHT 1200

static uint32_t g_buffer[WIDTH * HEIGHT];
// HW3 Part 3: Projection mode
static int perspective_mode = 0;
//variables for hw3 task 1
static int show_axes = 1;
static int show_bounding_box = 1;

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

// Part 5: application state controlled by UI
static float ring_density = 3.0f;
static float blue_level = 255.0f;

static bool g_meow_mode = false; //varuables for special effect after c key press
static uint8_t g_meow_r = 255;
static uint8_t g_meow_g = 255;
static uint8_t g_meow_b = 255;

// color of the next line
static float line_r = 255.0f;
static float line_g = 255.0f;
static float line_b = 255.0f;

//brush mode
static int brush_enabled = 0;
static bool brushing = false;

static int brush_prev_x = 0;
static int brush_prev_y = 0;

//line thickness
static float line_thickness = 1.0f;

struct Camera {//camera structure
    glm::vec3 position;
    glm::vec3 rotation;
};

struct BoundingBox {//structure for normalization of the object
    glm::vec3 min;
    glm::vec3 max;
};

struct Face {
    int v0;//vertexes
    int v1;
    int v2;
};

struct Line {
    int x0;
    int y0;
    int x1;
    int y1;
    uint32_t color;
    int thickness;
};

// HW3 Part 2: Camera
static Camera camera = {
    glm::vec3(0.0f, 0.0f, 1500.0f),
    glm::vec3(0.0f, 0.0f, 0.0f)
};

static std::vector<Line> lines;
static std::vector<glm::vec3> normalized_vertices;

// State of the line currently being drawn
static bool drawing = false;
static int start_x = 0;
static int start_y = 0;
static int current_x = 0;
static int current_y = 0;

BoundingBox find_bounding_box(const std::vector<glm::vec3>& vertices) {

    BoundingBox box;
    
    box.min = vertices[0];
    box.max = vertices[0];

    // we run on every vertex and compare it with minimum and maximum to find the max and min
    for (const glm::vec3& vertex : vertices) {

        box.min.x = std::min(box.min.x, vertex.x);
        box.min.y = std::min(box.min.y, vertex.y);
        box.min.z = std::min(box.min.z, vertex.z);

        box.max.x = std::max(box.max.x, vertex.x);
        box.max.y = std::max(box.max.y, vertex.y);
        box.max.z = std::max(box.max.z, vertex.z);
    }

    return box;
}

bool load_obj(const std::string& filename, std::vector<glm::vec3>& vertices, std::vector<Face>& faces) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        printf("Could not open OBJ file: %s\n", filename.c_str());
        return false;
    }

    std::string line;

    while (std::getline(file, line)) {

        std::stringstream ss(line);
        std::string type;

        ss >> type;

        // Vertex line: v x y z
        if (type == "v") {

            float x, y, z;
            ss >> x >> y >> z;

            vertices.push_back(glm::vec3(x, y, z));
        }

        // Face line: f v1 v2 v3
        else if (type == "f") {

            int a, b, c;
            ss >> a >> b >> c;

            // OBJ numbering starts from 1,
            // but C++ vectors start from 0.
            faces.push_back({a - 1, b - 1, c - 1});
        }
    }

    return true;
}

void draw_line(int x0, int y0, int x1, int y1, uint32_t color, int thickness) {

    int dx = abs(x1 - x0); //distance between x0 and x1
    int sx = x0 < x1 ? 1 : -1;//direction of the line on x, if 1 the line goes to the right, else to left

    int dy = -abs(y1 - y0);//distance between y0 and y1
    int sy = y0 < y1 ? 1 : -1;//direction of the line on y, if 1 the line goes up, else down

    int error = dx + dy;

    while (true) {

        if (x0 >= 0 && x0 < WIDTH && y0 >= 0 && y0 < HEIGHT) { //if x0 and y0 are in board range

            int radius = thickness / 2;
            // Draw a group of pixels around each Bresenham point
            // to create line thickness.
            for (int offset_y = -radius; offset_y <= radius; offset_y++) {
                for (int offset_x = -radius; offset_x <= radius; offset_x++) {

                    int px = x0 + offset_x;
                    int py = y0 + offset_y;

                    if (px >= 0 && px < WIDTH &&
                        py >= 0 && py < HEIGHT) {

                        g_buffer[py * WIDTH + px] = color;
                    }
                }
            }
        }

        if (x0 == x1 && y0 == y1)//we stop if the line is a dot
            break;

        int e2 = 2 * error;

        if (e2 >= dy) {//if the error from the ideal line is bigger than distanse of y, we should move on x
            error += dy;
            x0 += sx;// move line left or right according to coordinates
        }

        if (e2 <= dx) {//if the error from the ideal line is smaller than distanse of x, we should move on y
            error += dx;
            y0 += sy;// move line up or down according to coordinates
        }
    }
}

int main() 
{
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
    &world_scale_y,
    &world_scale_z
);
// Part 0: Simple GLM test
    glm::vec3 a(1.0f, 2.0f, 3.0f);
    glm::vec3 b(4.0f, 5.0f, 6.0f);

    glm::vec3 result = a + b;

    printf("GLM test: %.1f %.1f %.1f\n", result.x, result.y, result.z);

    // hw 2: Part 1: OBJ loading
    std::vector<glm::vec3> vertices;
    std::vector<Face> faces;

    bool obj_loaded =
    load_obj("../assignments/assets/Simple_pyramid.obj", vertices, faces);

    glm::vec3 model_translation(0.0f);
    float model_scale = 1.0f;

    if (obj_loaded && !vertices.empty()) {//if file is readable and it has vertexes

    printf("OBJ loaded successfully!\n");
    printf("Vertices: %zu\n", vertices.size());
    printf("Faces: %zu\n", faces.size());

    BoundingBox box = find_bounding_box(vertices);
    glm::vec3 size = box.max - box.min; // Size of the model
    glm::vec3 center = (box.min + box.max) * 0.5f; // Center of the model
    float largest_dimension = std::max(size.x, std::max(size.y, size.z));// Find the largest dimension
    model_scale = 1000.0f / largest_dimension;// Fit the model inside approximately 1000 units
    model_translation = -center;// Move the center of the model to the origin
    printf("Bounding box min: %.2f %.2f %.2f\n",box.min.x, box.min.y, box.min.z);
    printf("Bounding box max: %.2f %.2f %.2f\n",box.max.x, box.max.y, box.max.z);
    printf("Scale: %.2f\n", model_scale);

    if (obj_loaded && !vertices.empty()) {
        for (const glm::vec3& vertex : vertices) {

        glm::vec3 transformed = (vertex + model_translation) * model_scale;//trasforming each vertex
        normalized_vertices.push_back(transformed);//storing
    }
}
}
    // HW3 Part 1: bounding box of the normalized model
    BoundingBox normalized_box;

    if (!normalized_vertices.empty()) {
        normalized_box = find_bounding_box(normalized_vertices);
    }


    
    struct mfb_window *window =
      mfb_open_ex("MiniGUI Platform", WIDTH, HEIGHT, MFB_WF_RESIZABLE);
  if (!window)
    return 1;

  mu_Context *ctx = (mu_Context *)malloc(sizeof(mu_Context));
  mu_init(ctx);

  // Set font callbacks for microui
  ctx->text_width = [](mu_Font font, const char *str, int len) {
    return (len < 0 ? (int)strlen(str) : len) * 8;
  };
  ctx->text_height = [](mu_Font font) { return 8; };

  UIRenderer renderer(WIDTH, HEIGHT);

  // Set up char input callback for textbox input
  mfb_set_char_input_callback(
      [](struct mfb_window *w, unsigned int c) {
        extern void ui_bridge_char_input(struct mfb_window *, unsigned int);
        // C toggles MEOW mode and randomizes its color
        if (c == 'c' || c == 'C') {
          g_meow_mode = !g_meow_mode;

          g_meow_r = rand() % 256;
          g_meow_g = rand() % 256;
          g_meow_b = rand() % 256;

          printf("MEOW mode: %s\n", g_meow_mode ? "ON" : "OFF");

          return; // consume C
        }
        ui_bridge_char_input(w, c);
      },
      window);

  while (mfb_update_events(window) != MFB_STATE_EXIT) {
    // Input
    ui_bridge_input(ctx, window);

    // Part 6: interactive drawing

uint32_t current_color = MFB_RGB(
    (uint8_t)line_r,
    (uint8_t)line_g,
    (uint8_t)line_b
);

// The left 400 pixels are reserved for the UI.
// Drawing can only START to the right of this area.
bool mouse_on_canvas = ctx->mouse_pos.x >= 400;

// ----------straight line mode--------------


// Start a straight line only if Brush Mode is OFF
// and the mouse is inside the canvas.
if (!brush_enabled &&
    (ctx->mouse_pressed & MU_MOUSE_LEFT) &&
    !drawing &&
    mouse_on_canvas) {

    start_x = ctx->mouse_pos.x;
    start_y = ctx->mouse_pos.y;

    current_x = start_x;
    current_y = start_y;

    drawing = true;
}

// While drawing, update the end point.
if (!brush_enabled && drawing) {
    current_x = ctx->mouse_pos.x;
    current_y = ctx->mouse_pos.y;
}

// When mouse is released, store the finished line.
if (!brush_enabled &&
    drawing &&
    !(ctx->mouse_down & MU_MOUSE_LEFT)) {

    lines.push_back({
        start_x,
        start_y,
        current_x,
        current_y,
        current_color,
        (int)line_thickness
    });

    drawing = false;
}

// ---------brush mode-------------

// Start a new brush stroke.
if (brush_enabled &&
    mouse_on_canvas &&
    (ctx->mouse_pressed & MU_MOUSE_LEFT)) {

    brushing = true;

    // Remember where the mouse started.
    brush_prev_x = ctx->mouse_pos.x;
    brush_prev_y = ctx->mouse_pos.y;
}


// While the mouse button is held down,
// connect the previous mouse position to the new one.
if (brush_enabled &&
    brushing &&
    (ctx->mouse_down & MU_MOUSE_LEFT)) {

    int brush_x = ctx->mouse_pos.x;
    int brush_y = ctx->mouse_pos.y;

    // Only add a line segment if the mouse moved.
    if (brush_x != brush_prev_x ||
        brush_y != brush_prev_y) {

        lines.push_back({
            brush_prev_x,
            brush_prev_y,
            brush_x,
            brush_y,
            current_color,
            (int)line_thickness
        });

        // The current position becomes the starting
        // position of the next small segment.
        brush_prev_x = brush_x;
        brush_prev_y = brush_y;
    }
}


// Stop painting when the mouse button is released.
if (brushing &&
    !(ctx->mouse_down & MU_MOUSE_LEFT)) {

    brushing = false;
}

    // Scene Rendering (Background)
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
      int x = i % WIDTH;
      int y = i / WIDTH;

      int dx = x - WIDTH / 2;
      int dy = y - HEIGHT / 2;

      int distance = (int)sqrt((double)(dx * dx + dy * dy));

      uint8_t r = (uint8_t)(((int)(distance * ring_density)) % 256);
      uint8_t g = (uint8_t)(100);
      uint8_t b = (uint8_t)(((int)blue_level - distance) & 255);
      g_buffer[i] = MFB_RGB(r, g, b);
  }
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


//---------  World coordinate axes ----------------


    if (show_axes) {

        float axis_length = 300.0f;

        glm::vec4 origin(0.0f, 0.0f, 0.0f, 1.0f);// World origin

        // End points of the three world axes
        glm::vec4 x_axis(axis_length, 0.0f, 0.0f, 1.0f);
        glm::vec4 y_axis(0.0f, axis_length, 0.0f, 1.0f);
        glm::vec4 z_axis(0.0f, 0.0f, axis_length, 1.0f);

        // Transform world axes using the View matrix
        glm::vec4 view_origin = view_matrix * origin;
        glm::vec4 view_x_axis = view_matrix * x_axis;
        glm::vec4 view_y_axis = view_matrix * y_axis;
        glm::vec4 view_z_axis = view_matrix * z_axis;

        // Move view coordinates to screen coordinates
        int ox = (int)(view_origin.x + WIDTH / 2.0f);
        int oy = (int)(view_origin.y + HEIGHT / 2.0f);

        int xx = (int)(view_x_axis.x + WIDTH / 2.0f);
        int xy = (int)(view_x_axis.y + HEIGHT / 2.0f);

        int yx = (int)(view_y_axis.x + WIDTH / 2.0f);
        int yy = (int)(view_y_axis.y + HEIGHT / 2.0f);

        int zx = (int)(view_z_axis.x + WIDTH / 2.0f);
        int zy = (int)(view_z_axis.y + HEIGHT / 2.0f);

    // drawing the axes
        draw_line(ox, oy, xx, xy,MFB_RGB(255, 0, 0), 4);
        draw_line(ox, oy, yx, yy,MFB_RGB(0, 255, 0), 4);
        draw_line(ox, oy, zx, zy,MFB_RGB(0, 0, 255), 4);
    }
      
// ----------local coordinate axes-----------

if (show_axes) {

    float local_axis_length = 300.0f;

    // Local origin and axis endpoints
    glm::vec4 local_origin =view_matrix * final_matrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    glm::vec4 local_x =view_matrix * final_matrix * glm::vec4(local_axis_length, 0.0f, 0.0f, 1.0f);

    glm::vec4 local_y =view_matrix * final_matrix * glm::vec4(0.0f, local_axis_length, 0.0f, 1.0f);

    glm::vec4 local_z =view_matrix * final_matrix * glm::vec4(0.0f, 0.0f, local_axis_length, 1.0f);

    // Orthographic projection
    int ox = (int)(local_origin.x + WIDTH / 2.0f);
    int oy = (int)(local_origin.y + HEIGHT / 2.0f);

    int xx = (int)(local_x.x + WIDTH / 2.0f);
    int xy = (int)(local_x.y + HEIGHT / 2.0f);

    int yx = (int)(local_y.x + WIDTH / 2.0f);
    int yy = (int)(local_y.y + HEIGHT / 2.0f);

    int zx = (int)(local_z.x + WIDTH / 2.0f);
    int zy = (int)(local_z.y + HEIGHT / 2.0f);

    draw_line(ox, oy, xx, xy,MFB_RGB(255, 0, 0), 2);
    draw_line(ox, oy, yx, yy,MFB_RGB(0, 255, 0), 2);
    draw_line(ox, oy, zx, zy,MFB_RGB(0, 0, 255), 2);
}

    // Draw transformed OBJ wireframe
    for (const Face& face : faces) {
        glm::vec4 v0 = view_matrix * final_matrix * glm::vec4(normalized_vertices[face.v0], 1.0f);
        glm::vec4 v1 = view_matrix * final_matrix * glm::vec4(normalized_vertices[face.v1], 1.0f);
        glm::vec4 v2 = view_matrix * final_matrix * glm::vec4(normalized_vertices[face.v2], 1.0f);

        int x0, y0;
        int x1, y1;
        int x2, y2;

    if (perspective_mode) {

        v0 = perspective_matrix * v0;
        v1 = perspective_matrix * v1;
        v2 = perspective_matrix * v2;

        if (v0.w <= 0.0f ||v1.w <= 0.0f ||v2.w <= 0.0f) {
            continue;
    }

        v0 /= v0.w;
        v1 /= v1.w;
        v2 /= v2.w;

    // NDC [-1, 1] -> screen coordinates
        x0 = (int)((v0.x + 1.0f) * WIDTH  / 2.0f);
        y0 = (int)((v0.y + 1.0f) * HEIGHT / 2.0f);

        x1 = (int)((v1.x + 1.0f) * WIDTH  / 2.0f);
        y1 = (int)((v1.y + 1.0f) * HEIGHT / 2.0f);

        x2 = (int)((v2.x + 1.0f) * WIDTH  / 2.0f);
        y2 = (int)((v2.y + 1.0f) * HEIGHT / 2.0f);

} else {

        x0 = (int)(v0.x + WIDTH / 2.0f);
        y0 = (int)(v0.y + HEIGHT / 2.0f);

        x1 = (int)(v1.x + WIDTH / 2.0f);
        y1 = (int)(v1.y + HEIGHT / 2.0f);

        x2 = (int)(v2.x + WIDTH / 2.0f);
        y2 = (int)(v2.y + HEIGHT / 2.0f);
}

        draw_line(x0, y0, x1, y1,MFB_RGB(255, 255, 255), 2);
        draw_line(x1, y1, x2, y2,MFB_RGB(255, 255, 255), 2);
        draw_line(x2, y2, x0, y0,MFB_RGB(255, 255, 255), 2);

}
      
//------------Bounding Box---------------------------
if (show_bounding_box && !normalized_vertices.empty()) {

    // Create the 8 corners of the bounding box
    glm::vec3 corners[8] = {

        // Back side
        glm::vec3(normalized_box.min.x, normalized_box.min.y, normalized_box.min.z),
        glm::vec3(normalized_box.max.x, normalized_box.min.y, normalized_box.min.z),
        glm::vec3(normalized_box.max.x, normalized_box.max.y, normalized_box.min.z),
        glm::vec3(normalized_box.min.x, normalized_box.max.y, normalized_box.min.z),

        // Front side
        glm::vec3(normalized_box.min.x, normalized_box.min.y, normalized_box.max.z),
        glm::vec3(normalized_box.max.x, normalized_box.min.y, normalized_box.max.z),
        glm::vec3(normalized_box.max.x, normalized_box.max.y, normalized_box.max.z),
        glm::vec3(normalized_box.min.x, normalized_box.max.y, normalized_box.max.z)
    };

    // The 12 edges connecting the corners
    int edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };

    // Transform all corners using the same matrix as the model
    glm::vec4 transformed_corners[8];

    for (int i = 0; i < 8; i++) {
        transformed_corners[i] = view_matrix * final_matrix * glm::vec4(corners[i], 1.0f);
    }

    // Draw all 12 edges
    for (int i = 0; i < 12; i++) {

        glm::vec4 p0 = transformed_corners[edges[i][0]];
        glm::vec4 p1 = transformed_corners[edges[i][1]];

        // Orthographic projection
        int x0 = (int)(p0.x + WIDTH / 2.0f);
        int y0 = (int)(p0.y + HEIGHT / 2.0f);

        int x1 = (int)(p1.x + WIDTH / 2.0f);
        int y1 = (int)(p1.y + HEIGHT / 2.0f);

        draw_line(x0, y0,x1, y1,MFB_RGB(255, 255, 0),2);
    }
}
      // Draw all completed lines
for (const Line& line : lines) {
    draw_line(
        line.x0,
        line.y0,
        line.x1,
        line.y1,
        line.color,
        line.thickness
    );
}
// Draw temporary preview while dragging
if (drawing) {
    draw_line(
        start_x,
        start_y,
        current_x,
        current_y,
        current_color,
        (int)line_thickness
    );
}
      
    if (g_meow_mode) {
  // 5x7 pixel-font patterns for M E O W
  const char *letters[4][7] = {
      {
          "10001",
          "11011",
          "10101",
          "10101",
          "10001",
          "10001",
          "10001"
      },
      {
          "11111",
          "10000",
          "10000",
          "11110",
          "10000",
          "10000",
          "11111"
      },
      {
          "01110",
          "10001",
          "10001",
          "10001",
          "10001",
          "10001",
          "01110"
      },
      {
          "10001",
          "10001",
          "10001",
          "10101",
          "10101",
          "11011",
          "10001"
      }
  };

  int pixelSize = 25;
  int letterWidth = 5 * pixelSize;
  int spacing = pixelSize;

  int totalWidth = 4 * letterWidth + 3 * spacing;

  int startX = (WIDTH - totalWidth) / 2;
  int startY = (HEIGHT - 7 * pixelSize) / 2;

  uint32_t meowColor =
      MFB_RGB(g_meow_r, g_meow_g, g_meow_b);

  for (int letter = 0; letter < 4; letter++) {

    for (int row = 0; row < 7; row++) {

      for (int col = 0; col < 5; col++) {

        if (letters[letter][row][col] == '1') {

          int blockX =
              startX + letter * (letterWidth + spacing)
              + col * pixelSize;

          int blockY =
              startY + row * pixelSize;

          // Draw one large "pixel"
          for (int py = 0; py < pixelSize; py++) {
            for (int px = 0; px < pixelSize; px++) {

              int x = blockX + px;
              int y = blockY + py;

              if (x >= 0 && x < WIDTH &&
                  y >= 0 && y < HEIGHT) {

                g_buffer[y * WIDTH + x] = meowColor;
              }
            }
          }
        }
      }
    }
  }
}

    //  UI Logic
    static float slider_val = 50.0f;
    static float number_val = 3.14f;
    static int checkbox_a = 0;
    static int checkbox_b = 1;
    static char textbox_buf[128] = "edit me";
    static bool quit_requested = false;
    static int show_message = 0;

    mu_begin(ctx);

    // --- Widgets window ---
    if (mu_begin_window(ctx, "Widgets", mu_rect(20, 20, 360, 700))) {
      int w1[] = {-1};

      // label / text
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "mu_label: plain static text");
      mu_text(ctx, "mu_text: word-wrapped longer text that will reflow inside "
                   "the window width automatically.");
    
      // OBJ model information
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "OBJ Model Information:");

      char vertex_text[64];
      snprintf(vertex_text, sizeof(vertex_text),"Vertices: %zu", vertices.size());
      mu_label(ctx, vertex_text);

      char face_text[64];
      snprintf(face_text, sizeof(face_text),"Faces: %zu", faces.size());
      mu_label(ctx, face_text);

      // button
      mu_layout_row(ctx, 1, w1, 0);
      if (mu_button(ctx, "mu_button: click me")) {
        quit_requested = false; // just a reaction
      }

      // checkbox
      mu_layout_row(ctx, 1, w1, 0);
      mu_checkbox(ctx, "Show Coordinate Axes", &show_axes);
      mu_checkbox(ctx, "Show Bounding Box", &show_bounding_box);
      mu_checkbox(ctx, "Perspective Projection", &perspective_mode);

    //brush checkbox
      mu_layout_row(ctx, 1, w1, 0);
      mu_checkbox(ctx, "Brush Mode", &brush_enabled);

      // textbox
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "mu_textbox:");
      mu_textbox(ctx, textbox_buf, sizeof(textbox_buf));

      // slider
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "mu_slider (0-100):");
      mu_slider(ctx, &slider_val, 0, 100);

      // sliders for part 5
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "Ring Density:");
      mu_slider(ctx, &ring_density, 1.0f, 10.0f);

      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "Blue Level:");
      mu_slider(ctx, &blue_level, 0.0f, 255.0f);

      // sliders for lines:3
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "Line Red:");
      mu_slider(ctx, &line_r, 0.0f, 255.0f);

      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "Line Green:");
      mu_slider(ctx, &line_g, 0.0f, 255.0f);

      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "Line Blue:");
      mu_slider(ctx, &line_b, 0.0f, 255.0f);

    //slider for thickness
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "Line Thickness:");
      mu_slider(ctx, &line_thickness, 1.0f, 15.0f);

      // number
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "mu_number (step 0.1):");
      mu_number(ctx, &number_val, 0.1f);

      // header (collapsible section)
      if (mu_header(ctx, "mu_header: collapsible section")) {
        mu_layout_row(ctx, 1, w1, 0);
        mu_label(ctx, "Content inside the header.");
      }

      // treenode
      if (mu_begin_treenode(ctx, "mu_treenode: root")) {
        mu_layout_row(ctx, 1, w1, 0);
        mu_label(ctx, "child item A");
        if (mu_begin_treenode(ctx, "nested node")) {
          mu_layout_row(ctx, 1, w1, 0);
          mu_label(ctx, "deeply nested item");
          mu_end_treenode(ctx);
        }
        mu_end_treenode(ctx);
      }
        
      // quit button
      mu_layout_row(ctx, 1, w1, 0);
      if (mu_button(ctx, "Quit")) {
        quit_requested = true;
      }

    //clear screen button
    mu_layout_row(ctx, 1, w1, 0);

    if (mu_button(ctx, "Clear Screen")) {
        lines.clear();
    }
    //undo button
    mu_layout_row(ctx, 1, w1, 0);

    if (mu_button(ctx, "Undo")) {
        if (!lines.empty()) {
            lines.pop_back();
        }
    }
mu_end_window(ctx);
    }
    
          // --- Transformation Controls ---
if (mu_begin_window(ctx, "Transformations", mu_rect(1200, 20, 380, 1000))) {

    int wt[] = {-1};

    // -------- local --------

    mu_layout_row(ctx, 1, wt, 0);
    mu_label(ctx, "LOCAL TRANSFORMATIONS");

    // Local Translation
    mu_layout_row(ctx, 1, wt, 0);
    mu_label(ctx, "Local Translation X");
    mu_slider(ctx, &local_translation_x, -500.0f, 500.0f);

    mu_label(ctx, "Local Translation Y");
    mu_slider(ctx, &local_translation_y, -500.0f, 500.0f);

    mu_label(ctx, "Local Translation Z");
    mu_slider(ctx, &local_translation_z, -500.0f, 500.0f);

    // Local Rotation
    mu_layout_row(ctx, 1, wt, 0);
    mu_label(ctx, "Local Rotation X");
    mu_slider(ctx, &local_rotation_x, -180.0f, 180.0f);

    mu_label(ctx, "Local Rotation Y");
    mu_slider(ctx, &local_rotation_y, -180.0f, 180.0f);

    mu_label(ctx, "Local Rotation Z");
    mu_slider(ctx, &local_rotation_z, -180.0f, 180.0f);

    // Local Scale
    mu_layout_row(ctx, 1, wt, 0);
    mu_label(ctx, "Local Scale X");
    mu_slider(ctx, &local_scale_x, 0.1f, 3.0f);

    mu_label(ctx, "Local Scale Y");
    mu_slider(ctx, &local_scale_y, 0.1f, 3.0f);

    mu_label(ctx, "Local Scale Z");
    mu_slider(ctx, &local_scale_z, 0.1f, 3.0f);


    // -------- world --------

    mu_layout_row(ctx, 1, wt, 0);
    mu_label(ctx, "WORLD TRANSFORMATIONS");

    // World Translation
    mu_layout_row(ctx, 1, wt, 0);
    mu_label(ctx, "World Translation X");
    mu_slider(ctx, &world_translation_x, -500.0f, 500.0f);

    mu_label(ctx, "World Translation Y");
    mu_slider(ctx, &world_translation_y, -500.0f, 500.0f);

    mu_label(ctx, "World Translation Z");
    mu_slider(ctx, &world_translation_z, -500.0f, 500.0f);

    // World Rotation
    mu_layout_row(ctx, 1, wt, 0);
    mu_label(ctx, "World Rotation X");
    mu_slider(ctx, &world_rotation_x, -180.0f, 180.0f);

    mu_label(ctx, "World Rotation Y");
    mu_slider(ctx, &world_rotation_y, -180.0f, 180.0f);

    mu_label(ctx, "World Rotation Z");
    mu_slider(ctx, &world_rotation_z, -180.0f, 180.0f);

    // World Scale
    mu_layout_row(ctx, 1, wt, 0);
    mu_label(ctx, "World Scale X");
    mu_slider(ctx, &world_scale_x, 0.1f, 3.0f);

    mu_label(ctx, "World Scale Y");
    mu_slider(ctx, &world_scale_y, 0.1f, 3.0f);

    mu_label(ctx, "World Scale Z");
    mu_slider(ctx, &world_scale_z, 0.1f, 3.0f);

    // -------- camera --------

    mu_layout_row(ctx, 1, wt, 0);
    mu_label(ctx, "CAMERA");

    mu_label(ctx, "Camera Position X");
    mu_slider(ctx, &camera.position.x, -500.0f, 500.0f);

    mu_label(ctx, "Camera Position Y");
    mu_slider(ctx, &camera.position.y, -500.0f, 500.0f);

    mu_label(ctx, "Camera Position Z");
    mu_slider(ctx, &camera.position.z, -500.0f, 5000.0f);

    mu_label(ctx, "Camera Rotation X");
    mu_slider(ctx, &camera.rotation.x, -180.0f, 180.0f);

    mu_label(ctx, "Camera Rotation Y");
    mu_slider(ctx, &camera.rotation.y, -180.0f, 180.0f);

    mu_label(ctx, "Camera Rotation Z");
    mu_slider(ctx, &camera.rotation.z, -180.0f, 180.0f);

    mu_end_window(ctx);
}
    mu_end(ctx);

    if (quit_requested) {
      mfb_close(window);
      break;
    }


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
