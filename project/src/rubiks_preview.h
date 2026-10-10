#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cstdint>

// Draws a solved Rubik's Cube using the existing software triangle rasterizer.
// draw_filled_triangle(), WIDTH, HEIGHT, and MFB_RGB must be defined by main.cpp.
inline void draw_rubiks_preview(const glm::mat4& view, const glm::mat4& projection, const glm::mat4& final_matrix) {
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
    auto triangle = [&](const glm::vec3& a, const glm::vec3& b,const glm::vec3& c, uint32_t color) {//draws a triangle from 3 points and color
      //transforms the points via view and projection matrices
        glm::vec4 clip[3] = {glm::vec4 clip[3] = {
          projection * view * final_matrix * glm::vec4(a, 1),
          projection * view * final_matrix * glm::vec4(b, 1),
          projection * view * final_matrix * glm::vec4(c, 1)
};
        for (auto& p : clip) if (p.w <= 0.0f) return;// if the vertex is behind the camera we ignore it
        glm::vec3 screen[3];
        float depth[3];
        for (int i=0; i<3; ++i) {
            glm::vec3 ndc = glm::vec3(clip[i]) / clip[i].w;//perspective division as we did in previous assignments
            screen[i] = {(ndc.x+1)*WIDTH*0.5f, (ndc.y+1)*HEIGHT*0.5f, 0};//convertion to screen coordinates
            depth[i] = clip[i].w; // positive camera-space distance for perspective (for z buffer)
        }
        draw_filled_triangle((int)screen[0].x,(int)screen[0].y,depth[0],//the function that I used in hw to draw triangles
                             (int)screen[1].x,(int)screen[1].y,depth[1],
                             (int)screen[2].x,(int)screen[2].y,depth[2],color);
    };
    auto quad = [&](const glm::vec3 v[4], uint32_t color) {//to draw the cubes properly we split each cube into 2 triangles
        triangle(v[0],v[1],v[2],color);
        triangle(v[0],v[2],v[3],color);
    };
    for (int x=-1; x<=1; ++x)//goes over all of the possible coordinates in the cube
    for (int y=-1; y<=1; ++y)
    for (int z=-1; z<=1; ++z) {
        if (x==0 && y==0 && z==0) continue;// we ignore the middle cubie in the central section of the cube cause it doesnt matter
        glm::vec3 center(x*spacing,y*spacing,z*spacing);
        for (int f=0; f<6; ++f) {// for each cubie we loop through every face
            glm::vec3 body[4];
            for (int k=0; k<4; ++k)// we loop throught the corners of each face
                body[k] = center + corners[f][k]*halfSize;
            quad(body, MFB_RGB(20,22,28));//draws the face
            // Only outward faces have colored stickers.
            bool outer = (f==0 && x==1) || (f==1 && x==-1) ||
                         (f==2 && y==1) || (f==3 && y==-1) ||
                         (f==4 && z==1) || (f==5 && z==-1);
            if (!outer) continue;// if the face isnt outside we ignore it
            glm::vec3 sticker[4];
            for (int k=0; k<4; ++k) {
                glm::vec3 v = corners[f][k];
                // Scale the two tangential coordinates; offset along face normal.
                sticker[k] = center + (v - normals[f])*stickerHalf
                                   + normals[f]*stickerOffset;
            }
            quad(sticker, colors[f]);
        }
    }
}
