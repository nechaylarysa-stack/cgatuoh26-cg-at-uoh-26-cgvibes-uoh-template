# Assignment: Virtual Cameras and Projections

## Overview

In Assignment 2, you successfully loaded a 3D model, applied mathematical transformations, and orthographically flattened it to the screen. In this assignment, we will implement a proper virtual camera system. You will explore the View matrix, replace your basic orthographic projection with a true Perspective projection, and calculate geometric normals to prepare our models for lighting.

### Part 1: Coordinate Frames and Bounding Boxes

##### Task 1
Implement two visual debugging features in your renderer, and add UI checkboxes to toggle them on and off:

1. **Coordinate Axes:** Draw short, colored lines (e.g., Red for X, Green for Y, Blue for Z) originating from the center of the model to represent its Local axes, and a fixed set of axes at `(0,0,0)` to represent the World axes.

2. **Bounding Box:** Calculate the 8 corners of the object's 3D bounding box. Draw the wireframe of this box.
   *Test your implementation:* Transform your model. If you transform in the model frame, the model's axes should remain fixed relative to the model. If you transform in the world frame, the model's axes should transform alongside it!

**My answer:** 
Firstly I created checkboxes for the axes and the bounding box so the user could turn them off and on easily: 
```
//variables for hw3 task 1
static int show_axes = 1;
static int show_bounding_box = 1;
```
Then I begun by building the axes. I started with the easy set of axes first. The easiest are the world axes, hence they are only dependent on our screen and not the object itself that always transforms.
Below is the code for the world axes:
```
//---------  World coordinate axes ----------------


    if (show_axes) {

        float axis_length = 300.0f;

        glm::vec4 origin(0.0f, 0.0f, 0.0f, 1.0f);// World origin

        // End points of the three world axes
        glm::vec4 x_axis(axis_length, 0.0f, 0.0f, 1.0f);
        glm::vec4 y_axis(0.0f, axis_length, 0.0f, 1.0f);
        glm::vec4 z_axis(0.0f, 0.0f, axis_length, 1.0f);

        // Move world coordinates to screen coordinates
        int ox = (int)(origin.x + WIDTH / 2.0f);
        int oy = (int)(origin.y + HEIGHT / 2.0f);

        int xx = (int)(x_axis.x + WIDTH / 2.0f);
        int xy = (int)(x_axis.y + HEIGHT / 2.0f);

        int yx = (int)(y_axis.x + WIDTH / 2.0f);
        int yy = (int)(y_axis.y + HEIGHT / 2.0f);

        int zx = (int)(z_axis.x + WIDTH / 2.0f);
        int zy = (int)(z_axis.y + HEIGHT / 2.0f);

    // drawing the axes
        draw_line(ox, oy, xx, xy,MFB_RGB(255, 0, 0), 4);
        draw_line(ox, oy, yx, yy,MFB_RGB(0, 255, 0), 4);
        draw_line(ox, oy, zx, zy,MFB_RGB(0, 0, 255), 4);
    }
```
Then I implemented the local axes, by using the final matrix from previous assignment so the axes could transform just like the object.
Below is the code for the local axes:
```
// ----------local coordinate axes-----------

if (show_axes) {

    float local_axis_length = 300.0f;

    // Local origin and axis endpoints
    glm::vec4 local_origin =final_matrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    glm::vec4 local_x =final_matrix * glm::vec4(local_axis_length, 0.0f, 0.0f, 1.0f);

    glm::vec4 local_y =final_matrix * glm::vec4(0.0f, local_axis_length, 0.0f, 1.0f);

    glm::vec4 local_z =final_matrix * glm::vec4(0.0f, 0.0f, local_axis_length, 1.0f);

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

```


### Part 2: The Virtual Camera (View Matrix)

##### Background: The Camera Illusion

In computer graphics, a "camera" doesn't actually exist. To create the illusion of a camera moving forward into a scene, we actually move the entire 3D universe backward. This inverse transformation is called the **View Matrix**.

If a camera is positioned at $(C_x, C_y, C_z)$ and rotated by some angle, the View matrix applies the exact *opposite* translation and rotation to every vertex in the scene, effectively bringing the entire world into the camera's local coordinate space.

##### Task

Create a `Camera` object or struct. Give it position and rotation properties. Add UI sliders to control the camera's position and rotation in the world.
Construct the View matrix from these parameters (remembering to invert the transformation!) and multiply your model's vertices by this View matrix *after* the Model matrix but *before* the Projection matrix ($P \cdot V \cdot M \cdot v$). Verify that moving the camera left shifts the object to the right on your screen.

### Part 3: Perspective Projection

##### Background: The View Frustum and Perspective Divide

Orthographic projection (dropping the Z coordinate) makes architectural drafting easy, but it lacks depth—objects far away look the same size as objects close up.

A **Perspective Projection** maps a 3D truncated pyramid (the *frustum*) into a standardized 3D cube (Normalized Device Coordinates). It achieves the illusion of depth through the **Perspective Divide**: dividing the $X$ and $Y$ coordinates by the vertex's distance from the camera ($Z$ or $W$ in homogeneous coordinates). The further away a vertex is, the more its $X$ and $Y$ values are squashed toward the center of the screen.

##### Task

Use GLM (or derive the math yourself) to construct a Perspective Projection matrix. You will need to define a Field of View (FOV), an aspect ratio (based on your window size), and Near/Far clipping planes. Replace your orthographic projection with this new matrix. Add a UI button to toggle between Orthographic and Perspective modes. Load a mesh, move the camera away from it, and ensure the difference between the two projections is clearly visible.

### Part 4: Calculating Normals

##### Background: Which way is up?

To eventually calculate how light hits our object, we need to know which direction every polygon is facing. This direction is represented by a 3D unit vector called a **Normal**.

* A **Face Normal** is a single vector pointing perpendicular to the surface of a triangle.

* A **Vertex Normal** is a vector assigned to a vertex, usually calculated by averaging the face normals of all triangles sharing that vertex. This allows for smooth shading across jagged geometry.

##### Task

Write an algorithm to compute both the Face Normals and Vertex Normals for your loaded mesh. Use the cross product of the triangle's edges to find the face normal.
To verify your math is correct, implement a "Draw Normals" debug toggle in your UI. When enabled, use your `draw_line` function to draw short line segments pointing outward from the center of each face (for face normals) and from each vertex (for vertex normals). Make sure they transform correctly when you rotate the model!


