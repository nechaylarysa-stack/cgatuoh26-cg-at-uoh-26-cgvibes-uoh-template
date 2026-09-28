# Assignment: Basic Graphics and Immediate Mode GUI

## Overview

In this assignment, you will explore low-level computer graphics and Immediate Mode Graphical User Interfaces, and draw some lines and curves. You will manipulate a raw framebuffer to render graphics and modify a real-time rendering loop to understand how UI state is calculated and drawn independently of user input.

### Part 1: Manipulating the Framebuffer
##### Task
Open `main.cpp` and locate the Scene Rendering (Background) loop. Notice the `for` loop iterating over `WIDTH * HEIGHT`. Inside, it calculates the 2D `x` and `y` coordinates based on the 1D index `i`. It then assigns a 32-bit color integer to `g_buffer[i]` using the `MFB_RGB(r, g, b)` macro.

**Write a new mathematical expression for `r`, `g`, and `b` that will draw something different and creative.** Instead of just making a solid color, try generating gradients, shapes, or interesting patterns using the `x`, `y`, and `i` variables. Write a new expression that utilizes both the `x` and `y` coordinates to create a visible 2D pattern (e.g., a gradient, a checkerboard, or concentric circles). For full credit, the pattern cannot be a solid color or a 1D horizontal/vertical strip. Note: You are welcome to use AI to assist you in coming up with the math for these visual patterns!

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

##### Task
In `main.cpp` under the `mu_begin(ctx)` block, add a new interactive widget (such as a button or checkbox) that simply prints a message to the console or toggles a static text label within the MicroUI window. This will allow you to practice Immediate Mode syntax and UI layout without worrying about the broader application state yet.

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

##### Background: The Event Loop and Callbacks
To maintain a smooth framerate and interactive application, the program executes a strict chronological sequence of operations many times per second—commonly referred to as the **event loop**. In our code, this primary loop is controlled by `while (mfb_update_events(window) != MFB_STATE_EXIT)`. 

But how does the operating system interface with input devices (like a keyboard) and pass that data into our loop? It relies on **callbacks**. A callback is a function that you pass to the window manager so the OS knows exactly what code to execute when a hardware interrupt (like a keystroke) occurs. 

For example, in `main.cpp`, we register a function using `mfb_set_char_input_callback`. When a user presses a key, the OS triggers this callback, which calls `ui_bridge_char_input` to save the keystroke into a temporary array called `g_pending_text`. Later, during the sequential event loop, `ui_bridge_input` checks this array and feeds the captured input into the UI system. This separation ensures that unpredictable user inputs are safely synchronized with the strict timing of our rendering loop.

##### Task
In `main.cpp`, locate the character input callback (`mfb_set_char_input_callback`) just above the main event loop. **Be creative and intercept the input pipeline to trigger a custom visual effect:** write custom logic inside the callback so that pressing a specific key on your keyboard dynamically alters an application state variable, instantly changing the background pattern, randomizing the colors, or toggling a visual effect on the screen! 

*Note on event consumption:* If you intercept an event here, you must decide whether to "consume" it (stop the UI from seeing it) or pass it along to the UI bridge (`g_pending_text`) so normal widgets still function correctly.

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

##### Background: Abstract State vs. Visual Rendering
As you have learned, MicroUI generates an abstract list of commands, such as drawing rectangles, text, or icons. However, MicroUI itself has absolutely no concept of pixels or how to draw them to your screen.

To bridge this gap, we use a separate rendering system defined in our `UIRenderer` class. The `UIRenderer::render` function processes the queue of commands generated by MicroUI via a `while (mu_next_command(ctx, &cmd))` loop, translating these instructions into actual pixel manipulations.

This separation of concerns means that the physical boundaries where a button detects a "click" are calculated entirely independently from where the visual representation of that button is drawn to the framebuffer.

##### Task
Open `ui_renderer.cpp` and locate the rendering methods like `draw_rect` or `draw_text`. **Implement a custom visual transformation or stylistic override**—such as shifting coordinates, adding a wave effect, or injecting a color glitch—specifically when calculating the 1D index and assigning pixels to `m_buffer`. Recompile and attempt to interact with your transformed UI elements on the screen. 

After applying your visual offset, attempt to click on a button. In your code comments or submission text, explain exactly *why* clicking the visual representation of your shifted button no longer works, and describe where you must put your mouse cursor to successfully trigger it.

---

### Part 5: Binding UI to Application State

##### Background: Memory Pointers and State Mutation
Because Immediate Mode widgets are destroyed and recreated from scratch every single frame, they are fundamentally "stateless"—meaning they cannot store their own internal memory.

To function interactively, widgets instead take pointers to external variables that live in your application logic. When you drag a slider, the widget doesn't update its own internal "slider value"; rather, it directly mutates the value at the specific memory address you provided using the C++ address-of operator (`&`).

##### Task
In `main.cpp`, observe how the variable `slider_val` is passed into the slider widget using `mu_slider(ctx, &slider_val, 0, 100);`. **Design a new interactive feature** by declaring your own custom application state variables and binding them to brand new widgets (like sliders or checkboxes) inside the `mu_begin_window` block. Connect these newly bound variables to your background rendering loop from Part 1 so that interacting with your UI dynamically morphs, recolors, or animates the creative visual pattern you generated.

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

### Part 7: Pair Programming Extensions

*Students working in pairs are required to complete the following three extensions to receive full credit.*

##### 1. Bresenham's Circle Algorithm
*   **Background:** The principles of Bresenham's line algorithm can be extended to draw circles by exploiting 8-way symmetry—you only need to calculate the math for one octant and mirror the pixels to the other seven.
*   **Task:** Implement a `draw_circle(int xc, int yc, int r, uint32_t color)` function. Update your AI-assisted UI from Part 6 to support a "Circle Mode," allowing the user to click and drag to dynamically define the center and radius of a circle. 

##### 2. Performance Profiling: Bresenham vs. Naive
*   **Background:** Bresenham's algorithm was designed to avoid expensive floating-point arithmetic. However, modern CPUs process floating-point math significantly faster than hardware from the 1960s. Is the strict integer optimization still noticeably faster today?
*   **Task:** Implement a "naive" line drawing function that uses standard `float` math (calculating the slope $m$ and evaluating $y = mx + b$). Write a benchmarking routine that draws 100,000 random lines using both algorithms. Log the execution times to the console to definitively compare their performance on your specific hardware.

##### 3. Anti-Aliasing: Xiaolin Wu's Algorithm
*   **Background:** Bresenham's algorithm produces "aliased" (jagged) lines. Xiaolin Wu's line algorithm solves this by drawing pairs of pixels that straddle the mathematical line, distributing the color intensity based on the exact fractional distance to the line's true center. 
*   **Task:** Implement Xiaolin Wu's line algorithm. Because our assignment ignores the alpha channel (as established in Part 1). Note what happens when you draw a line on top of another line and attempt to fix the issue. Finally, add a UI toggle to instantly switch between Bresenham and Xiaolin Wu modes to visually compare the results.
