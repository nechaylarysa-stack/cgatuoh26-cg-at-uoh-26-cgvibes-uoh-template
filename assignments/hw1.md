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

##### Background: Memory Pointers and State Mutation
Because Immediate Mode widgets are destroyed and recreated from scratch every single frame, they are fundamentally "stateless"—meaning they cannot store their own internal memory.

To function interactively, widgets instead take pointers to external variables that live in your application logic. When you drag a slider, the widget doesn't update its own internal "slider value"; rather, it directly mutates the value at the specific memory address you provided using the C++ address-of operator (`&`).

##### Task
In `main.cpp`, observe how the variable `slider_val` is passed into the slider widget using `mu_slider(ctx, &slider_val, 0, 100);`. **Design a new interactive feature** by declaring your own custom application state variables and binding them to brand new widgets (like sliders or checkboxes) inside the `mu_begin_window` block. Connect these newly bound variables to your background rendering loop from Part 1 so that interacting with your UI dynamically morphs, recolors, or animates the creative visual pattern you generated.

**My answer:** I added 2 new sliders to the program that control the background from part 1. The first slider controls the ring density in the background and the second slider controls the blue level of the background.

To do so I first added 2 corresponding static variables outside the main loop so the sliders could take pointers to them.

```
// Part 5: application state controlled by UI
static float ring_density = 3.0f;
static float blue_level = 255.0f;
```

Then I altered the background loop so the values there would depend on the sliders values:

```
```

And of course I constructed the sliders themselves as the example slider was made in main.cpp:

```
```

### Part 6: Interactive Line Drawing App

##### Background: Implementing the Algorithm
In class, we discussed the theory behind **Bresenham's Line Algorithm** and how it elegantly approximates a straight line on a discrete pixel grid using only fast integer math. Now, it is time to translate that theory into a working renderer.

As you recall, calculating lines that go in any arbitrary direction means handling all eight possible octants (e.g., steep slopes vs. shallow slopes, drawing left-to-right vs. right-to-left). This can quickly lead to a messy explosion of `if-else` statements and redundant code. Your code should adhere to the *DRY* principle.

##### Task: Write the Line Function
Write a new function, such as `draw_line(int x0, int y0, int x1, int y1, uint32_t color)`, that calculates the pixels of a line between `(x0, y0)` and `(x1, y1)` using your optimized Bresenham's implementation. For each calculated pixel, assign the `color` to the correct 1D index in your `g_buffer`. Test it by hardcoding a few lines into your background render loop to verify the math is working across different slopes and directions.

##### Task: AI-Assisted UX Planning
Now, you must bridge the Immediate Mode UI concepts from Parts 2-5 with your new `draw_line` function to create an interactive drawing tool. But before you write the code, you need to design the interaction. 

**Use an AI assistant to brainstorm the User Experience (UX) for drawing.** Prompt the AI to discuss the pros, cons, and logic of different ways a user might draw a line with a mouse. 
*   Does the user click once to set the start point, and click again to set the end point? 
*   Do they click and hold, drag the mouse, and release to finalize the line? 
*   If they drag, how do you manage the "state" so the line is previewed but not permanently drawn until the mouse is released?

Reason through these approaches, choose the one you think makes the best application, and implement it using MicroUI's input and mouse state variables.

##### Task: The Creative Canvas
Combine everything you have built into a useful, interactive tool. The baseline requirement is that the user can interactively draw multiple permanent lines onto the screen. 

However, you are highly encouraged to push the limits of your architecture. An ambitious implementation might feature:
*   A UI panel with sliders to control the RGB values of the current line.
*   A "Clear Screen" button.
*   An automated "spirograph" mode that draws algorithmic lines based on UI slider parameters.
*   Logic to handle drawing continuous, connected lines (a brush tool).

Design an interface and visual result that you are proud of!
