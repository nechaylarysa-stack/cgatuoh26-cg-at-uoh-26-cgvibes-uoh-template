# Assignment: Basic Graphics and Immediate Mode GUI

## Overview

In this assignment, you will explore low-level computer graphics and Immediate Mode Graphical User Interfaces, and draw some lines and curves. You will manipulate a raw framebuffer to render graphics and modify a real-time rendering loop to understand how UI state is calculated and drawn independently of user input.

### Part 1: Manipulating the Framebuffer
##### Task

**My answer:** I changed the background to be in a pattern of concentric circles. 

```
for (int i = 0; i < WIDTH * HEIGHT; i++) {
  int x = i % WIDTH; #we find the coordinate of x of the pixel
  int y = i / WIDTH; #we find the coordinate of y of the pixel

  int dx = x - WIDTH / 2; #we find the distance of coordinate x from the x of the center
  int dy = y - HEIGHT / 2; #we find the distance of coordinate y from the y of the center

  int distance = (int)sqrt((double)(dx * dx + dy * dy)); #we use pythagorean theorem to find the distance from the center of the screen to this pixel

  uint8_t r = (uint8_t)((distance * 3) % 256); #we increase the red value each time the distance from center increases and when we go over maximum value (256) we start over
  uint8_t g = (uint8_t)(100); #green stays the same value
  uint8_t b = (uint8_t)((255 - distance) & 255); #we decrease the blue value each time the distance from center increases and when we go over maximum value (256) we start over

  g_buffer[i] = MFB_RGB(r, g, b);
}
```
We get the effect of circular red rings because the red value is `3*distance from centre` therefore each point in the same distance from centre receives the same red value therefore creating a circle, and then after repeating increases we pass the mark of 255 which is the highest possible value of red and it makes us return back to lower values which creates the illusion that the red ring ended.

We get the effect of the blue color in the inner part of the red rings because of the calculation of the value of blue which is `(255 - distance) & 255` this calculation makes the blue brighter in the beginning because the distance from center is small and therefore `255-distance` has a bigger value, unlike red that has technically a bigger value the farther it is from the center. And then the blue relapses because it gets out of bounds of 255 much like red.

We also have a green tint effect because each pixel has the same green value

<img width="1595" height="1047" alt="Image" src="https://github.com/user-attachments/assets/92a02a36-971d-4120-bfbb-9307ec8c9416" />

### Part 2: Immediate Mode UI Declaration

**My answer:**
```
// custom interactive widget
      mu_layout_row(ctx, 1, w1, 0);

      if (mu_button(ctx, "Toggle message")) {
          show_message = !show_message;
          printf("Toggle message button clicked\n");
        }

      if (show_message) {
          mu_layout_row(ctx, 1, w1, 0);
          mu_label(ctx, "Hello World!!1");
        }
```

<img width="415" height="527" alt="Image" src="https://github.com/user-attachments/assets/555819d8-f3ec-41ab-af35-e34c6add1a8b" />


As the code suggests I added a button with the name "Toggle message" that has control over a "show_message" flag that updates to true once our new button has been pressed. When the program sees that the flag = true it prints "Hello World!!1" beneath.

### Part 3: The Real-Time Graphics Loop and Input Handling

**My answer:**
```
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

      // Everything else still goes to the UI
      ui_bridge_char_input(w, c);
    },
    window);
```
This code adds the condition of if you press key 'c' or 'C' then the callback function changes the "meow effect" flag to be true if pressed once and false if pressed again and also randomizes the color for the pixel art in the effect by picking 3 random red, green and blue values.
```
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
```
In this code we create the pixel art of the world "meow" on the background 

![Nano Renderer MEOW effect](./assets/background_effect.png)

### Part 4: UI Architecture & The Renderer Bridge

**My answer:**
```
void UIRenderer::draw_rect(mu_Rect rect, mu_Color color) {
    uint32_t c = to_uint32(color);
    int x1 = std::max({rect.x, m_clip_rect.x, 0});
    int y1 = std::max({rect.y, m_clip_rect.y, 0});
    int x2 = std::min({rect.x + rect.w, m_clip_rect.x + m_clip_rect.w, m_width});
    int y2 = std::min({rect.y + rect.h, m_clip_rect.y + m_clip_rect.h, m_height});

    for (int y = y1; y < y2; y++) {
        for (int x = x1; x < x2; x++) {

        // Visual transformation:
        // Shift rectangle pixels 200 pixels DOWN.
            int shifted_y = y + 200;

            if (shifted_y >= 0 && shifted_y < m_height) {
                m_buffer[shifted_y * m_width + x] = c;
        }
    }
}
}
```

In this code I took the original draw rect from the ui renderer and altered it only by shifting the all of the y coordinates down by 200, which made all of the rectangles that held the text before drop down so it doesn't match anymore with the text as we can see in the picture.
This changes only the visual representation of the UI. MicroUI's internal layout and input coordinates remain unchanged. As a result, the visible button is drawn 200 pixels to the down of its logical hitbox.
Therefore clicking directly on the shifted visual representation will not correspond to the button's original interaction area that isn't controlled by the renderer. 
To press the button, the mouse cursor has to be at the original place of the button, which is 200 pixels upwards from the rectangles or since we didn't switch the position of the text yet, right on the text since it remains at it's original place.

---

### Part 5: Binding UI to Application State

**My answer:** I added 2 new sliders to the program that control the background from part 1. The first slider controls the ring density in the background and the second slider controls the blue level of the background.

To do so I first added 2 corresponding static variables outside the main loop so the sliders could take pointers to them.

Ring_density starts at 3.0, which is the same multiplier that I originally used for the red component of my background. blue_level starts at 255.0, which corresponds to the original starting blue value.

```
// Part 5: application state controlled by UI
static float ring_density = 3.0f;
static float blue_level = 255.0f;
```

Then I altered the background loop so the values there would depend on the sliders values:

```
while (mfb_update_events(window) != MFB_STATE_EXIT) {
    // Input
    ui_bridge_input(ctx, window);

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

```
The calculation of distance is still the same as in Part 1. For every pixel, I calculate its distance from the centre of the screen. Pixels at the same distance from the centre receive the same calculated color values, which produces the concentric circular pattern.

The important difference is that the red calculation now uses: distance * ring_density
instead of the original fixed calculation: distance * 3

So now changing ring_density changes how quickly the red value increases as the distance from the centre increases. With a smaller ring_density, the color changes more slowly and the rings become wider. With a larger value, the red component cycles through its range more quickly, producing more densely packed rings.

The blue calculation was also changed from a fixed starting value of 255 to the variable blue_level: (uint8_t)(((int)blue_level - distance) & 255)

Because of that now the user can change the starting blue component interactively. Moving the Blue Level slider changes the amount and distribution of blue in the background.

And of course I constructed the sliders themselves as the example slider was made in main.cpp:

```
// sliders for part 5
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "Ring Density:");//name of slider
      mu_slider(ctx, &ring_density, 1.0f, 10.0f);//pointer to external variable 

      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "Blue Level:");// name of slider
      mu_slider(ctx, &blue_level, 0.0f, 255.0f);//pointer to external variable
```

### Part 6: Interactive Line Drawing App

##### Task: Write the Line Function
**My answer:**
Below is my code for the line algorithm:
```
void draw_line(int x0, int y0, int x1, int y1, uint32_t color) {

    int dx = abs(x1 - x0); //distance between x0 and x1
    int sx = x0 < x1 ? 1 : -1;//direction of the line on x, if 1 the line goes to the right, else to left

    int dy = -abs(y1 - y0);//distance between y0 and y1
    int sy = y0 < y1 ? 1 : -1;//direction of the line on y, if 1 the line goes up, else down

    int error = dx + dy;

    while (true) {

        if (x0 >= 0 && x0 < WIDTH && y0 >= 0 && y0 < HEIGHT) { //if x0 and y0 are in board range

            g_buffer[y0 * WIDTH + x0] = color; //we place the color we want on the forst coordinate of the line
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
```
This function first measures the distance between the two coordinates given. Then we calculate the error value by adding up the distances, later on we use double the error to decide whether the algorithm should move in the x direction, the y direction, or both and after each movement we of course update the error. Additionally there are of course ifs for edge cases.

And here are the hard coded test commands to check if it works properly:
```
draw_line(600, 450, 1200, 650, MFB_RGB(255, 0, 0));     
draw_line(800, 350, 1000, 950, MFB_RGB(0, 255, 0));     
draw_line(1200, 400, 600, 750, MFB_RGB(0, 0, 255));     
draw_line(1100, 900, 650, 400, MFB_RGB(255, 255, 0));   
```
As you can see in the following picture the lines are drawn in an appropriate way.


##### Task: AI-Assisted UX Planning
**My answer:** 
There are 2 main options for the possible UX planning for line drawing algorithm.
The first and easier to implement version is a two-click system where the user makes two clicks on the screen where they want the two edges of the line to go. The obvious advantage of this method is that it is simple to implement because the program mainly needs to remember whether the first point has already been selected. But the main con of this implementation is that it is an unnatural drawing implementation compared tp regular canvas apps that use the dragging like brush method for drawing.

The second option was a click-drag-release system. The user presses the mouse button to establish the starting point, holds the button while moving the mouse to choose the endpoint, and releases the button to finalize the line. The con here is that it requires more application state, but the advantage of this method is that it behaves more like your normal drawing tools and allows the user to see the result before committing to it.

Because of the second option's similarity to regular drawing tools and the more friendly user experience of it I chose it as our UX plan.

To implement this, the application needs to know wether we are actively drawing or not and for that we will use state variables that remember whether a line is currently being drawn, where it started, and where the mouse currently is:

```
bool drawing = false; //drawing or not flag

int start_x; //start x coordinate
int start_y; //start y coordinate

int current_x; //current mouse position x
int current_y; //current mouse position y
```

When the mouse button is initially pressed, its coordinates are copied into start_x and start_y, and drawing becomes true. While the button remains pressed, current_x and current_y follow the mouse. The program can then call draw_line() using the stored starting point and the current mouse position:

```
if (drawing) {
    draw_line(start_x, start_y, current_x, current_y, preview_color);
}
```

This code generates only a **preview** line and not the actual end results since we haven't released the mouse yet. Since the frame buffer is regenerated every frame, the old preview disappears and a new preview is drawn using the latest mouse coordinates.

When the mouse button is released, the final start and end coordinates are stored as a permanent line. The `drawing` variable is then set back to false.

For more practicality we are also adding a new structure to the code the "line" struct:

```
struct Line {
    int x0;
    int y0;
    int x1;
    int y1;
    uint32_t color;
};
```
Here are the new code parts that I added to draw the preview and final lines and interact with the mouse:
```
    // Part 6: interactive line drawing

    current_x = ctx->mouse_pos.x;
    current_y = ctx->mouse_pos.y;

    // Mouse was just pressed
    if ((ctx->mouse_pressed & MU_MOUSE_LEFT) && !drawing) {
        start_x = ctx->mouse_pos.x;
        start_y = ctx->mouse_pos.y;

        current_x = start_x;
        current_y = start_y;

        drawing = true;
    }

    // Mouse was released
    if (drawing &&
        !(ctx->mouse_down & MU_MOUSE_LEFT)) {

        lines.push_back({
            start_x,
            start_y,
            current_x,
            current_y,
            MFB_RGB(255, 255, 255)
        });

        drawing = false;
    }
```
```
// Draw all completed lines
for (const Line& line : lines) {
    draw_line(
        line.x0,
        line.y0,
        line.x1,
        line.y1,
        line.color
    );
}
```
```
// Draw temporary preview while dragging
if (drawing) {
    draw_line(
        start_x,
        start_y,
        current_x,
        current_y,
        MFB_RGB(255, 255, 255)
    );
}
```
As we can see everything here works according to our previously described idea.
Below is a gif that shows that the code indeed works:


##### Task: The Creative Canvas
**My answer:**

From the previous task we already have all of the baseline requirements, but in this task I will still implement the following things:
- RGB sliders for drawing (similar to those implemented for the background)
- "Clear Screen" button
- "Undo" button

We will start by adding the RGB sliders. For that we need first to add 3 state variables for 3 of the sliders:
```
static float line_r = 255.0f;
static float line_g = 255.0f;
static float line_b = 255.0f;
```
After establishing the variables I changed the color of new lines to be the color made from the sliders instead of the set color that it was before. For example in lines.push_back the MFB_RGB(255, 255, 255) that previously was the set color for every line changed to "current_color" that I defined as:
```
uint32_t current_color = MFB_RGB(
    (uint8_t)line_r,
    (uint8_t)line_g,
    (uint8_t)line_b
);
```
And then the sliders were added immediately after, similar to sliders that we implemented priorly:
```
mu_layout_row(ctx, 1, w1, 0);
mu_label(ctx, "Line Red:");
mu_slider(ctx, &line_r, 0.0f, 255.0f);

mu_layout_row(ctx, 1, w1, 0);
mu_label(ctx, "Line Green:");
mu_slider(ctx, &line_g, 0.0f, 255.0f);

mu_layout_row(ctx, 1, w1, 0);
mu_label(ctx, "Line Blue:");
mu_slider(ctx, &line_b, 0.0f, 255.0f);
```
Now we jump to the second and the third conditions that we wanted to fulfill:
The "Clear Screen" and "Undo" buttons operate relatively similarly because we originally store all of our lines in a vector, that means that all we need to do to erase all of the lines is simply to clear that vector, which is a known vector function. And for "Undo" we need to pop the latest line from the vector, which is also an existing function for vectors hooray!

```
  // quit button
      mu_layout_row(ctx, 1, w1, 0);
      if (mu_button(ctx, "Quit")) {
        quit_requested = true;
      }

      mu_end_window(ctx);
    }

    //clear screen button
    mu_layout_row(ctx, 1, w1, 0);

    if (mu_button(ctx, "Clear Screen")) {
        lines.clear();//erasing the whole vector lines
    }
    //undo button
    mu_layout_row(ctx, 1, w1, 0);

    if (mu_button(ctx, "Undo")) {
        if (!lines.empty()) {
            lines.pop_back();// popping the last line
        }
    }
```
