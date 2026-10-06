# Assignment: Wireframe Viewer and Geometric Transformations

## Overview

In this assignment, you will transition from drawing 2D pixels to manipulating 3D geometry. You will load a 3D mesh into memory, project its vertices onto your 2D screen, and draw it using the line-drawing algorithm you built in Assignment 1. Finally, you will implement mathematical transformations (scaling, rotation, and translation) and wire them up to your Immediate Mode GUI to manipulate the 3D object in real-time.

### Part 0: Introduction to GLM

##### Task 0

**My Answer:**
Here are the verifications that the gel was inserted properly:

```
// Part 0: Simple GLM test
glm::vec3 a(1.0f, 2.0f, 3.0f);
glm::vec3 b(4.0f, 5.0f, 6.0f);

glm::vec3 result = a + b;

printf("GLM test: %.1f %.1f %.1f\n",
       result.x, result.y, result.z);
```
Confirmation of compiling and the example properly working in terminal:

![glm proof](./assets/glm_proof.png)

### Part 1: Loading and Inspecting 3D Data

##### Task 1

**My Answer:**

Write a function that loads an `.obj` file. To check your code, create an `.obj` file that contains an object with up to 10 vertices and faces, load it, and display the number of faces and vertices in the GUI and see if it matches the content of the file. You may display more information as seem necessary.

This section is the loader test via terminal, after the terminal test runs smoothly I will show off the results in the GUI:

Load object function:
```
struct Face {
    int v0;//vertexes
    int v1;
    int v2;
};

std::vector<glm::vec3> vertices;
std::vector<Face> faces;

bool load_obj(const std::string& filename, std::vector<glm::vec3>& vertices, std::vector<Face>& faces) {//receives the //name of the file, vector of vertices and vector of faces

    std::ifstream file(filename);//reads data from the file that we are trying to load

    if (!file.is_open()) {//if ifstream can't open the file we print an error message
        printf("Could not open OBJ file: %s\n", filename.c_str());
        return false;
    }

    std::string line;

    while (std::getline(file, line)) {//reading the file line by line

        std::stringstream ss(line);//devides the line to pieces so we can process each part separately
        std::string type;//stores the first part of the line which is the type 

        ss >> type;// moves to the next part of the line

        // Vertex line: v x y z
        if (type == "v") {//if it is a vertex line

            float x, y, z;
            ss >> x >> y >> z;//the next 3 parts of the line are x y z coordinates

            vertices.push_back(glm::vec3(x, y, z));//we store the vertex that we discovered in vertex vector
        }

        // Face line: f v1 v2 v3
        else if (type == "f") {//if it is a face line

            int a, b, c;
            ss >> a >> b >> c;//same as we did with vertexes

            // OBJ numbering starts from 1,
            // but C++ vectors start from 0.
            faces.push_back({a - 1, b - 1, c - 1});//we store the new faces in faces vector
        }
    }

    return true;
}
```
This function read every line in the file and if it starts with 'v' it is a vertex description so it stores the x y z coordinates in it, and if it starts with 'f' it is a face description so we store all of the faces in it. If the file can't be read the function returns false and a message.

test code:
```
bool obj_loaded = load_obj("assets/test.obj", vertices, faces);//true if the file has been read //else false

if (obj_loaded) {//if the file was read successfully prints the following messages
    printf("OBJ loaded successfully!\n");
    printf("Vertices: %zu\n", vertices.size());
    printf("Faces: %zu\n", faces.size());
}
```
This test code prints "OBJ loaded successfully! Vertices: "num_of vertices" Faces: "num_of_faces"" (in terminal)

test obj:
```
v -1.0 0.0 -1.0
v  1.0 0.0 -1.0
v  1.0 0.0  1.0
v -1.0 0.0  1.0
v  0.0 2.0  0.0

f 1 2 5
f 2 3 5
f 3 4 5
f 4 1 5
f 1 2 3
f 1 3 4
```
terminal results:

![obj result](./assets/result_of_obj_func.png)

As we can see in the terminal there are 5 vertices and 6 faces just like in the object file, which means everything works successfully!

Now all that's left is to display this information in the GUI by adding new widgets:

```
// OBJ model information
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "OBJ Model Information:");

      char vertex_text[64];
      snprintf(vertex_text, sizeof(vertex_text),"Vertices: %zu", vertices.size());
      mu_label(ctx, vertex_text);

      char face_text[64];
      snprintf(face_text, sizeof(face_text),"Faces: %zu", faces.size());
      mu_label(ctx, face_text);
```
result:

![obj result](./assets/obj_info_in_gui.png)


### Part 2: Normalization and the Viewport Transform

When you load a mesh, its vertex coordinates are completely arbitrary. A model of an ant might have coordinates ranging from $-0.01$ to $0.01$, while a model of a city block might range from $-5000$ to $5000$, but it could also be the opposite. There are no guarantees.

If you try to draw these raw coordinates directly to your framebuffer (which likely ranges from $0$ to $1000$ pixels), the whole object might be contained in a single pixel, or be entirely off-screen. To fix this, we must apply a *temporary* debugging transformation to scale and center the object so it fits nicely inside our window.

##### Task

Write an algorithm to find the bounding box of your loaded mesh (the minimum and maximum $x$, $y$, and $z$ values). Using this information, calculate a uniform scale factor and a translation vector to map the model's vertices so that they fit comfortably within your window's dimensions (e.g., scaling them up/down to around $0-1000$ and centering them). In your report, write a brief explanation of the mathematical logic you used to calculate this specific bounding-box-to-window transformation.

### Part 3: Orthographic Projection and Wireframe Rendering

Our screen is a 2D grid of pixels, but our mesh exists in 3D space. To draw it, we must mathematically flatten the 3D vertices into 2D points.

The simplest way to do this is an **Orthographic Projection**, which essentially ignores depth. To orthographically project a point $(x, y, z)$ straight onto the 2D plane of your monitor, you simply drop the $z$-coordinate and use $(x, y)$ to draw to the screen.

##### Task

Iterate over all the faces (triangles) in the mesh. For each triangle, retrieve its three 3D vertices, drop one of the coordinates (typically $z$) to project them into 2D, and draw the three connecting edges using the `draw_line` function you wrote in Assignment 1. You should now see a static wireframe model clearly displayed on your screen! Place a screenshot of your rendered wireframe model in your report.

### Part 4: Transformation Matrices & Immediate Mode GUI

To move, rotate, or scale a 3D object, we multiply its vertices by $4 \times 4$ transformation matrices. A complex movement is achieved by creating separate basic matrices for Scale ($S$), Rotation ($R$), and Translation ($T$), and multiplying them together into a single Model Matrix ($M$).

Furthermore, transformations can occur in different "frames of reference." You can transform an object relative to its own center (**Local Transformations**) or relative to the center of the universe (**World Transformations**).

##### Task

In your rendering loop, add new GUI widgets (such as sliders or input boxes) to control the $X, Y, Z$ parameters for both Local and World transformations. You should have separate UI controls for:

* Local Translation, Local Rotation, Local Scale

* World Translation, World Rotation, World Scale

Take a screenshot of the GUI layout you designed and include it in your report.

### Part 5: Applying Transformations

In linear algebra, matrix multiplication is not commutative ($A \cdot B \neq B \cdot A$). The order in which you apply transformations drastically changes the visual result.

If you *Translate then Rotate*, the object moves to a new position and then revolves around the origin like a planet orbiting the sun. If you *Rotate then Translate*, the object spins in place like a top, and is then moved to its new position. This distinction is the core difference between World and Local frame transformations.

##### Task

Compute the final transformation matrices based on your UI slider values, and apply them (by multiplying) to your mesh's vertices *before* you perform the orthographic projection and draw the lines. Verify that the model transforms interactively as you move the sliders. Show two screenshots in your report comparing the difference between:

1. Translating in the model (local) frame and then rotating in the world frame.

2. Translating in the world frame and then rotating in the local (model) frame.

### Part 6: Interactive Input Modifiers

While GUI sliders are excellent for precise control, modern 3D applications allow users to interact with the scene directly using the mouse or keyboard. By intercepting input events before they reach the UI, we can increment or decrement our transformation state variables dynamically.

##### Task

Implement one approach for modifying the basic transformations using direct keyboard or mouse input. For example, you might map the arrow keys to World Translation, or map holding the left mouse button and dragging to Local Rotation. Describe your chosen input method and how it modifies the transformation state in your report.



* **Task:** Implement a *second* approach for modifying transformations using the mouse (so you have two total, fulfilling the "two approaches" requirement for pairs). For example, if you mapped mouse-dragging to rotation in Part 6, map the mouse scroll wheel to uniformly scale the active object, or map right-click-dragging to translation. Describe both implementations in your report.
