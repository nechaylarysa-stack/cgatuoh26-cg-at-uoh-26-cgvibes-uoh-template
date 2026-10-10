#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cstdint>
#include <cstdint>
#include <vector>
#include <cmath>

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
}

struct RubiksCubie {
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
            result.push_back(p);
        }
        return result;
    }();
    return pieces;
}

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
}

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
}

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
}

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
    );
}

// Draws a solved Rubik's Cube using the existing software triangle rasterizer.
// draw_filled_triangle(), WIDTH, HEIGHT, and MFB_RGB must be defined by main.cpp.

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

    auto glassTriangle = [&](const glm::vec3& a,
                             const glm::vec3& b,
                             const glm::vec3& c,
                             uint32_t color,
                             float alpha) {

        glm::vec4 clip[3] = {
            projection * view * final_matrix * glm::vec4(a, 1.0f),
            projection * view * final_matrix * glm::vec4(b, 1.0f),
            projection * view * final_matrix * glm::vec4(c, 1.0f)
        };

        for (const auto& p : clip)
            if (p.w <= 0.0f) return;

        glm::vec3 screen[3];
        float depth[3];

        for (int i = 0; i < 3; ++i) {
            glm::vec3 ndc = glm::vec3(clip[i]) / clip[i].w;
            screen[i] = {
                (ndc.x + 1.0f) * WIDTH * 0.5f,
                (ndc.y + 1.0f) * HEIGHT * 0.5f,
                0.0f
            };
            depth[i] = clip[i].w;
        }

        float minX = std::max(0.0f, std::floor(std::min({
            screen[0].x, screen[1].x, screen[2].x
        })));
        float maxX = std::min(float(WIDTH - 1), std::ceil(std::max({
            screen[0].x, screen[1].x, screen[2].x
        })));
        float minY = std::max(0.0f, std::floor(std::min({
            screen[0].y, screen[1].y, screen[2].y
        })));
        float maxY = std::min(float(HEIGHT - 1), std::ceil(std::max({
            screen[0].y, screen[1].y, screen[2].y
        })));

        auto edge = [](const glm::vec3& a,
                       const glm::vec3& b,
                       float x, float y) {
            return (x - a.x) * (b.y - a.y) -
                   (y - a.y) * (b.x - a.x);
        };

        float area = edge(screen[0], screen[1],
                          screen[2].x, screen[2].y);
        if (std::abs(area) < 0.00001f) return;

        for (int y = int(minY); y <= int(maxY); ++y) {
            for (int x = int(minX); x <= int(maxX); ++x) {
                float px = x + 0.5f;
                float py = y + 0.5f;

                float w0 = edge(screen[1], screen[2], px, py) / area;
                float w1 = edge(screen[2], screen[0], px, py) / area;
                float w2 = edge(screen[0], screen[1], px, py) / area;

                if (w0 < 0 || w1 < 0 || w2 < 0)
                    continue;

                float z = w0 * depth[0] +
                          w1 * depth[1] +
                          w2 * depth[2];

                int index = y * WIDTH + x;

                if (z <= z_buffer[index]) {
                    g_buffer[index] = rubiks_blend_color(
                        g_buffer[index], color, alpha
                    );
                }
            }
        }
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

    auto glassHighlight = [&](const glm::vec3& a,
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

        constexpr int radius = 9;
        constexpr float sigma = 3.5f;

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

                    float distanceSquared =
                        float(ox * ox + oy * oy);

                    float alpha =
                        0.13f * std::exp(
                            -distanceSquared /
                            (2.0f * sigma * sigma)
                        );

                    g_buffer[index] = rubiks_blend_color(
                        g_buffer[index],
                        color,
                        alpha
                    );
                }
            }
        }
    };

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

    
    struct NeonEdge {
        glm::vec3 a;
        glm::vec3 b;
        uint32_t color;
    };

    std::vector<NeonEdge> neonEdges;
    struct GlassFace {
    glm::vec3 vertices[4];
    uint32_t color;
    float alpha;
    float depth;
    bool sticker;
};

std::vector<GlassFace> glassFaces;

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
                collectGlassFace(body,MFB_RGB(120, 190, 225),0.28f,false);
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
            
            else if (glassMode) {
                collectGlassFace(
                sticker,
                p.stickers[f],
                0.60f,
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
