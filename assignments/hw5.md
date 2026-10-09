# Assignment: Lighting, Materials, and Shading

## Overview

Our solid models currently look flat and artificial. In the real world, our perception of 3D shape comes from how light interacts with surfaces. In this final assignment, you will implement the classic **Phong Reflection Model**, calculate the interaction between virtual lights and surface normals, and use interpolation to create smooth, realistic shading.

### Part 1: Light Sources and Material Properties

##### Task 1

**My answer:**

I begun by creating two new structs, PointLight and Material, as the assignment suggested. The PointLight struct contains the position of the light in 3D space and the RGB color components: ambient, diffuse, and specular. The Material struct contains the same three RGB properties, but this time they describe how the object's surface responds to the different types of lighting.For all these values I used vec3 to store 3 separate values inside. In position, these represent (x,y,z) and for the colors they represent the red, green, and blue components.

```
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
};
```
Afterwards I created two static variables light and material. I placed light at (0, 0, 1000) and initially set all its color components to (1, 1, 1), which represents white light at full intensity. Material I initialized as (0.8, 0.5, 0.3) as its ambient color, which gives the object a brownish-orange color. The diffuse and specular properties are initially set to white.

```
static PointLight light = {
    glm::vec3(0.0f, 0.0f, 1000.0f),
    glm::vec3(1.0f, 1.0f, 1.0f),
    glm::vec3(1.0f, 1.0f, 1.0f),
    glm::vec3(1.0f, 1.0f, 1.0f)
};

static Material material = {
    glm::vec3(0.8f, 0.5f, 0.3f),
    glm::vec3(1.0f, 1.0f, 1.0f),
    glm::vec3(1.0f, 1.0f, 1.0f)
};
```
After that, I created the calculate_ambient_color() function, which performs the ambient lighting calculation. To do ambient lighting we multiply the ambient color of the light by the ambient color of the material. Since both values are glm::vec3, the multiplication happens separately for the red, green, and blue components. After calculating the result, I use glm::clamp() to make sure that all the values stay between 0 and 1 to not go outside of the RGB range. Finally, I multiply each component by 255 and convert it to uint8_t, since our framebuffer uses RGB values between 0 and 255. Then I combine them into one color using MFB_RGB() and return it.
```
uint32_t calculate_ambient_color() {

    glm::vec3 ambient_color = light.ambient * material.ambient;
    ambient_color = glm::clamp(ambient_color,glm::vec3(0.0f),glm::vec3(1.0f));
    return MFB_RGB((uint8_t)(ambient_color.r * 255.0f),(uint8_t)(ambient_color.g * 255.0f),(uint8_t)(ambient_color.b * 255.0f));
}
```
Then of course I updated the rendering loop to use the new ambient color instead of the random triangle colors from the previous assignment.

```
if (show_filled_triangles || show_z_buffer) {

    uint32_t color = calculate_ambient_color();
    draw_filled_triangle(x0, y0, z0,x1, y1, z1,x2, y2, z2,color);
}
```
Finally, I added a new window called Lighting to the GUI using mu_begin_window(), just like we did with the previous controls:
```
if (mu_begin_window(ctx, "Lighting", mu_rect(1200, 430, 350, 600))) {

    int w[] = {-1};

    mu_layout_row(ctx, 1, w, 0);
    mu_label(ctx, "LIGHT POSITION");//light position for x,y,z

    mu_label(ctx, "Light X");
    mu_slider(ctx, &light.position.x, -2000.0f, 2000.0f);

    mu_label(ctx, "Light Y");
    mu_slider(ctx, &light.position.y, -2000.0f, 2000.0f);

    mu_label(ctx, "Light Z");
    mu_slider(ctx, &light.position.z, -2000.0f, 2000.0f);

    mu_layout_row(ctx, 1, w, 0);
    mu_label(ctx, "AMBIENT LIGHT COLOR");//RGB controls 

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

    mu_end_window(ctx);
}
```
result:



### Part 2: Flat Shading (Diffuse Lighting)

##### Background: Lambert's Cosine Law

Diffuse lighting relies on **Lambert's Cosine Law**: the brightness of a surface is proportional to the cosine of the angle between the surface normal and the direction of the light source. Mathematically, this is achieved by taking the **Dot Product** of the normalized Light Direction vector and the normalized Face Normal vector.

##### Task

Calculate the Diffuse component for each triangle. To do this using **Flat Shading**, calculate the lighting equation *once* per triangle using the Face Normal and the center point of the triangle. Add this Diffuse result to your Ambient result. Your model will now have shading, but will look heavily faceted, like a jewel or a low-poly aesthetic, because every pixel on a given triangle receives the exact same color.

### Part 3: Specular Highlights

##### Background: The Reflection Vector

To simulate shininess, we must calculate the Specular component. This requires knowing the direction the light *reflects* off the surface, and comparing it to the direction of the *Camera* (the View vector). If the reflected light points straight into the camera, we draw a bright highlight.

##### Task

Implement a function to compute the Reflection vector of the light against the surface normal. Use this vector, along with the View vector and the material's "shininess" exponent, to calculate the Specular component. Add this to the Ambient and Diffuse components.
To verify your math, use your `draw_line` function to draw both the incoming Light Vector and the outgoing Reflection Vector from the center of a few faces on your model. Include a screenshot of these debug vectors in your report.

### Part 4: Phong Shading (Per-Pixel Shading)

##### Background: Interpolating Normals

Flat shading looks unrealistic for curved surfaces (like spheres). To make a blocky mesh look perfectly smooth, we must calculate the lighting equation for *every single pixel* rather than once per face. This is called **Phong Shading**.

To do this, we don't use the Face Normal. Instead, we take the three **Vertex Normals** of the triangle, and use the exact same Barycentric Coordinates we used for rasterization to *interpolate* a brand new normal for the specific pixel we are currently drawing.

##### Task

Modify your rasterization loop. For every pixel:

1. Interpolate the 3D position of the pixel using barycentric weights.

2. Interpolate the normal of the pixel using barycentric weights.

3. Normalize the newly interpolated normal vector.

4. Calculate the full Ambient + Diffuse + Specular lighting equation using these interpolated values.

Render the result. Your jagged, low-poly model should now look incredibly smooth and realistically lit!

