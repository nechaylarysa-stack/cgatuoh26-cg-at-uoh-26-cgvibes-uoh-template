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
#include <limits>

extern "C" {
#include "microui.h"
}
#include "ui_bridge.h"
#include "ui_renderer.h"

#define WIDTH 1600
#define HEIGHT 1200

static uint32_t g_buffer[WIDTH * HEIGHT];
static float z_buffer[WIDTH * HEIGHT];
static int use_phong_shading = 1;
static int show_reflection_vectors = 0;
static int show_z_buffer = 0;
//HW4 part 2: triangle filling
static int show_filled_triangles = 0;
//HW4 part 1: rectangle filling
static int show_bounding_rectangles = 0;
// HW3 part 4: normals
static int draw_face_normals = 0;
static int draw_vertex_normals = 0;
// HW3 Part 3: Projection mode
static int perspective_mode = 1;
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

static std::vector<Line> lines;
static std::vector<glm::vec3> normalized_vertices;
static std::vector<glm::vec3> face_normals;
static std::vector<glm::vec3> vertex_normals;
static std::vector<Face> faces;

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
void calculate_normals(const std::vector<Face>& faces) {

    face_normals.clear();
    vertex_normals.clear();

    vertex_normals.resize(
        normalized_vertices.size(),
        glm::vec3(0.0f)
    );

    printf("Calculating normals...\n");
    printf("Faces received: %zu\n", faces.size());
    printf("Vertices available: %zu\n", normalized_vertices.size());

    for (const Face& face : faces) {

        // Safety check
        if (face.v0 < 0 || face.v1 < 0 || face.v2 < 0 ||
            face.v0 >= normalized_vertices.size() ||
            face.v1 >= normalized_vertices.size() ||
            face.v2 >= normalized_vertices.size()) {

            printf("Invalid face indices: %d %d %d\n",
                   face.v0, face.v1, face.v2);

            continue;
        }

        glm::vec3 v0 = normalized_vertices[face.v0];
        glm::vec3 v1 = normalized_vertices[face.v1];
        glm::vec3 v2 = normalized_vertices[face.v2];

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;

        glm::vec3 cross_product =
            glm::cross(edge1, edge2);

        // Avoid normalizing a zero-length vector
        if (glm::length(cross_product) == 0.0f) {
            face_normals.push_back(glm::vec3(0.0f));
            continue;
        }

        glm::vec3 normal =
            glm::normalize(cross_product);

        face_normals.push_back(normal);

        vertex_normals[face.v0] += normal;
        vertex_normals[face.v1] += normal;
        vertex_normals[face.v2] += normal;
    }

    for (glm::vec3& normal : vertex_normals) {

        if (glm::length(normal) > 0.0f) {
            normal = glm::normalize(normal);
        }
    }

    printf("Normals calculated!\n");
    printf("Face normals: %zu\n", face_normals.size());
    printf("Vertex normals: %zu\n", vertex_normals.size());
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

void draw_filled_rectangle(int x_min, int y_min, int x_max, int y_max, uint32_t color) {

    // Keep rectangle inside the screen
    x_min = std::max(0, x_min);
    y_min = std::max(0, y_min);

    x_max = std::min(WIDTH - 1, x_max);
    y_max = std::min(HEIGHT - 1, y_max);

    // Fill every pixel inside the rectangle
    for (int y = y_min; y <= y_max; y++) {
        for (int x = x_min; x <= x_max; x++) {
            g_buffer[y * WIDTH + x] = color;
        }
    }
}
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


void visualize_z_buffer() {

    float min_depth = std::numeric_limits<float>::infinity();
    float max_depth = -std::numeric_limits<float>::infinity();

    // Find the depth range of visible pixels
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        if (std::isfinite(z_buffer[i])) {
            min_depth = std::min(min_depth, z_buffer[i]);
            max_depth = std::max(max_depth, z_buffer[i]);
        }
    }

    if (!std::isfinite(min_depth)) {
        return;
    }

    float range = max_depth - min_depth;

    for (int i = 0; i < WIDTH * HEIGHT; i++) {

        if (!std::isfinite(z_buffer[i])) {
            g_buffer[i] = MFB_RGB(0, 0, 0);
            continue;
        }

        float normalized = 0.0f;

        if (range > 0.0f) {
            normalized = (z_buffer[i] - min_depth) / range;
        }

        // Closer = darker, farther = lighter
        uint8_t gray = (uint8_t)(normalized * 255.0f);

        g_buffer[i] = MFB_RGB(gray, gray, gray);
    }
}

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
}
#include "rubiks_preview.h"
            
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
    load_obj("../assignments/assets/low_poly_sphere.obj", vertices, faces);

    // HW4 Part 2: Generate one random color for each triangle
    std::vector<uint32_t> triangle_colors;

    for (size_t i = 0; i < faces.size(); i++) {
        uint8_t r = rand() % 256;
        uint8_t g = rand() % 256;
        uint8_t b = rand() % 256;

        triangle_colors.push_back(MFB_RGB(r, g, b));
}
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
    calculate_normals(faces);
    printf("Faces: %zu\n", faces.size());
    printf("Face normals: %zu\n", face_normals.size());
    printf("Vertex normals: %zu\n", vertex_normals.size());
    printf("Normalized vertices: %zu\n", normalized_vertices.size());
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
if (brush_enabled &&brushing &&(ctx->mouse_down & MU_MOUSE_LEFT)) {

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
if (brushing &&!(ctx->mouse_down & MU_MOUSE_LEFT)) {

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
    // Clear Z-buffer once per frame
    std::fill(z_buffer,z_buffer + WIDTH * HEIGHT,std::numeric_limits<float>::infinity());
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
    if (false) for (size_t i = 0; i < faces.size(); i++) {
        const Face& face = faces[i];
        // View-space vertices for rendering
        glm::vec4 v0 = view_matrix * final_matrix * glm::vec4(normalized_vertices[face.v0], 1.0f);
        glm::vec4 v1 = view_matrix * final_matrix * glm::vec4(normalized_vertices[face.v1], 1.0f);
        glm::vec4 v2 = view_matrix * final_matrix * glm::vec4(normalized_vertices[face.v2], 1.0f);

        // World-space vertices for lighting
        glm::vec4 world_v0 = final_matrix * glm::vec4(normalized_vertices[face.v0], 1.0f);
        glm::vec4 world_v1 = final_matrix * glm::vec4(normalized_vertices[face.v1], 1.0f);
        glm::vec4 world_v2 = final_matrix * glm::vec4(normalized_vertices[face.v2], 1.0f);

        // Transform vertex normals into world space
        glm::vec3 n0 = glm::normalize(normal_matrix * vertex_normals[face.v0]);
        glm::vec3 n1 = glm::normalize(normal_matrix * vertex_normals[face.v1]);
        glm::vec3 n2 = glm::normalize(normal_matrix * vertex_normals[face.v2]);

        // Depth of each vertex
        float z0 = -v0.z;
        float z1 = -v1.z;
        float z2 = -v2.z;

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

        if (show_filled_triangles || show_z_buffer) {
            if (use_phong_shading) {
                draw_phong_triangle(x0, y0, z0,x1, y1, z1,x2, y2, z2,glm::vec3(world_v0),glm::vec3(world_v1),glm::vec3(world_v2),n0, n1, n2);
    }
            else {
                uint32_t color = calculate_flat_shading(glm::vec3(world_v0),glm::vec3(world_v1),glm::vec3(world_v2));
                draw_filled_triangle(x0, y0, z0,x1, y1, z1,x2, y2, z2,color);
    }
}
            
        else if (show_bounding_rectangles) {

    // Find minimum and maximum screen coordinates
            int x_min = std::min(x0, std::min(x1, x2));
            int x_max = std::max(x0, std::max(x1, x2));

            int y_min = std::min(y0, std::min(y1, y2));
            int y_max = std::max(y0, std::max(y1, y2));

    // Fill the triangle's bounding rectangle
            draw_filled_rectangle(x_min, y_min,x_max, y_max, triangle_colors[i]);

} else {

    // Original wireframe rendering
    draw_line(x0, y0, x1, y1, MFB_RGB(255, 255, 255), 2);
    draw_line(x1, y1, x2, y2, MFB_RGB(255, 255, 255), 2);
    draw_line(x2, y2, x0, y0, MFB_RGB(255, 255, 255), 2);
}
// Debug reflection vectors
if (show_reflection_vectors) {
    float vector_length = 150.0f;
    // Draw vectors for a few faces
    for (size_t i = 0; i < faces.size(); i += 50) {
        const Face& face = faces[i];
        // World-space vertices
        glm::vec3 v0_world = glm::vec3(final_matrix * glm::vec4(normalized_vertices[face.v0], 1.0f));
        glm::vec3 v1_world = glm::vec3(final_matrix * glm::vec4(normalized_vertices[face.v1], 1.0f));
        glm::vec3 v2_world = glm::vec3(final_matrix * glm::vec4(normalized_vertices[face.v2], 1.0f));
        // Triangle center
        glm::vec3 center = (v0_world + v1_world + v2_world) / 3.0f;
        glm::vec3 edge1 = v1_world - v0_world;
        glm::vec3 edge2 = v2_world - v0_world;
        if (glm::length(glm::cross(edge1, edge2)) < 0.000001f) {
            continue;
        }
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
        // Incoming light direction
        glm::vec3 light_direction = light.position - center;
        if (glm::length(light_direction) < 0.000001f) {
            continue;
        }
        glm::vec3 incoming = -glm::normalize(light_direction);
        // Reflected direction
        glm::vec3 reflection = glm::normalize(calculate_reflection(incoming, normal));

        // Points for the debug lines
        glm::vec3 incoming_start = center - incoming * vector_length;
        glm::vec3 reflection_end = center + reflection * vector_length;

        // Transform into view space
        glm::vec4 p0 = view_matrix * glm::vec4(center, 1.0f);
        glm::vec4 p1 = view_matrix * glm::vec4(incoming_start, 1.0f);
        glm::vec4 p2 = view_matrix * glm::vec4(reflection_end, 1.0f);

        // Convert to screen coordinates
        auto project_debug = [&](glm::vec4 p) -> glm::vec2 {

            if (perspective_mode) {
                p = perspective_matrix * p;

                if (p.w <= 0.0f) {
                    return glm::vec2(-10000.0f);
                }

                p /= p.w;

                return glm::vec2((p.x + 1.0f) * WIDTH / 2.0f,(p.y + 1.0f) * HEIGHT / 2.0f);
            }

            return glm::vec2(
                p.x + WIDTH / 2.0f,
                p.y + HEIGHT / 2.0f
            );
        };

        glm::vec2 screen_center = project_debug(p0);
        glm::vec2 screen_incoming = project_debug(p1);
        glm::vec2 screen_reflection = project_debug(p2);

        // incoming light
        draw_line((int)screen_incoming.x, (int)screen_incoming.y,(int)screen_center.x, (int)screen_center.y,MFB_RGB(255, 255, 0), 2);

        // outgoing reflection
        draw_line((int)screen_center.x, (int)screen_center.y,(int)screen_reflection.x, (int)screen_reflection.y,MFB_RGB(0, 255, 255), 2);
    }
}

}
//---------------drawing face normals-----------------------------
if (draw_face_normals) {

    float normal_length = 100.0f;

    for (size_t i = 0; i < faces.size(); i++) {

        const Face& face = faces[i];

        glm::vec3 v0 = normalized_vertices[face.v0];
        glm::vec3 v1 = normalized_vertices[face.v1];
        glm::vec3 v2 = normalized_vertices[face.v2];

        // Center of triangle
        glm::vec3 center = (v0 + v1 + v2) / 3.0f;

        // End point of normal
        glm::vec3 end = center + face_normals[i] * normal_length;

        // Transform both points
        glm::vec4 p0 = view_matrix * final_matrix * glm::vec4(center, 1.0f);
        glm::vec4 p1 = view_matrix * final_matrix * glm::vec4(end, 1.0f);

        int x0 = (int)(p0.x + WIDTH / 2.0f);
        int y0 = (int)(p0.y + HEIGHT / 2.0f);

        int x1 = (int)(p1.x + WIDTH / 2.0f);
        int y1 = (int)(p1.y + HEIGHT / 2.0f);

        draw_line(x0, y0,x1, y1,MFB_RGB(255, 0, 255),2);
    }
}
//------------------draw vertex normals-------------
if (draw_vertex_normals) {

    float normal_length = 100.0f;

    for (size_t i = 0; i < normalized_vertices.size(); i++) {

        glm::vec3 start = normalized_vertices[i];
        glm::vec3 end = start + vertex_normals[i] * normal_length;
        glm::vec4 p0 = view_matrix * final_matrix * glm::vec4(start, 1.0f);
        glm::vec4 p1 = view_matrix * final_matrix * glm::vec4(end, 1.0f);

        int x0 = (int)(p0.x + WIDTH / 2.0f);
        int y0 = (int)(p0.y + HEIGHT / 2.0f);

        int x1 = (int)(p1.x + WIDTH / 2.0f);
        int y1 = (int)(p1.y + HEIGHT / 2.0f);

        draw_line(x0, y0, x1, y1, MFB_RGB(0, 255, 255),2);
    }
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
    draw_rubiks_preview(glm::lookAt(glm::vec3(950.0f, 750.0f, 1600.0f),glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),perspective_matrix);
            
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
    if (mu_begin_window(ctx, "Widgets", mu_rect(20, 20, 360, 1000))) {
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
      mu_checkbox(ctx, "Draw Face Normals", &draw_face_normals);
      mu_checkbox(ctx, "Draw Vertex Normals", &draw_vertex_normals);
      mu_checkbox(ctx, "Bounding Rectangle Debug", &show_bounding_rectangles);
      mu_checkbox(ctx, "Filled Triangles", &show_filled_triangles);
      mu_checkbox(ctx, "Phong Shading", &use_phong_shading);
      mu_checkbox(ctx, "Show Z-Buffer", &show_z_buffer);
      mu_checkbox(ctx, "Show Reflection Vectors", &show_reflection_vectors);

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
if (mu_begin_window(ctx, "Transformations", mu_rect(1200, 20, 350, 400))) {

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

if (mu_begin_window(ctx, "Lighting", mu_rect(1200, 430, 350, 600))) {

    int w[] = {-1};

    mu_layout_row(ctx, 1, w, 0);
    mu_label(ctx, "LIGHT POSITION");

    mu_label(ctx, "Light X");
    mu_slider(ctx, &light.position.x, -2000.0f, 2000.0f);

    mu_label(ctx, "Light Y");
    mu_slider(ctx, &light.position.y, -2000.0f, 2000.0f);

    mu_label(ctx, "Light Z");
    mu_slider(ctx, &light.position.z, -2000.0f, 2000.0f);

    mu_layout_row(ctx, 1, w, 0);
    mu_label(ctx, "AMBIENT LIGHT COLOR");

    mu_label(ctx, "Ambient Red");
    mu_slider(ctx, &light.ambient.r, 0.0f, 1.0f);

    mu_label(ctx, "Ambient Green");
    mu_slider(ctx, &light.ambient.g, 0.0f, 1.0f);

    mu_label(ctx, "Ambient Blue");
    mu_slider(ctx, &light.ambient.b, 0.0f, 1.0f);

    mu_layout_row(ctx, 1, w, 0);
    mu_label(ctx, "DIFFUSE LIGHT COLOR");

    mu_label(ctx, "Diffuse Red");
    mu_slider(ctx, &light.diffuse.r, 0.0f, 1.0f);

    mu_label(ctx, "Diffuse Green");
    mu_slider(ctx, &light.diffuse.g, 0.0f, 1.0f);

    mu_label(ctx, "Diffuse Blue");
    mu_slider(ctx, &light.diffuse.b, 0.0f, 1.0f);

    mu_layout_row(ctx, 1, w, 0);
    mu_label(ctx, "SPECULAR LIGHT COLOR");

    mu_label(ctx, "Specular Red");
    mu_slider(ctx, &light.specular.r, 0.0f, 1.0f);

    mu_label(ctx, "Specular Green");
    mu_slider(ctx, &light.specular.g, 0.0f, 1.0f);

    mu_label(ctx, "Specular Blue");
    mu_slider(ctx, &light.specular.b, 0.0f, 1.0f);

    mu_label(ctx, "MATERIAL SHININESS");
    mu_slider(ctx, &material.shininess, 1.0f, 128.0f);


    mu_end_window(ctx);
}
    mu_end(ctx);

    if (quit_requested) {
      mfb_close(window);
      break;
    }

    if (show_z_buffer) {
        visualize_z_buffer();
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
