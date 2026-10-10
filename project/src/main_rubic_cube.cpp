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
#include <chrono>
#include <cmath>
#include <cstring>

extern "C" {
#include "microui.h"
}
#include "ui_bridge.h"
#include "ui_renderer.h"

#define WIDTH 1600
#define HEIGHT 1200

static uint32_t g_buffer[WIDTH * HEIGHT];
static float z_buffer[WIDTH * HEIGHT];

 // Rubik's Cube visual effects
static int rubiks_normal = 1;
static int rubiks_neon = 0;
static int rubiks_glass = 0;

static int use_phong_shading = 0; // Flat shading is much faster in software; UI can enable Phong
static int perspective_mode = 1;

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
        ui_bridge_char_input(w, c);
      },
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

       // Fast, uniform charcoal background.
    std::fill(g_buffer, g_buffer + WIDTH * HEIGHT, MFB_RGB(32, 35, 42));
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
