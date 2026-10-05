#include "MiniFB.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" {
#include "microui.h"
}
#include "ui_bridge.h"
#include "ui_renderer.h"

#define WIDTH 1600
#define HEIGHT 1200

static uint32_t g_buffer[WIDTH * HEIGHT];

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

#include <vector>

struct Line {
    int x0;
    int y0;
    int x1;
    int y1;
    uint32_t color;
};

static std::vector<Line> lines;

// State of the line currently being drawn
static bool drawing = false;
static int start_x = 0;
static int start_y = 0;
static int current_x = 0;
static int current_y = 0;

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

int main() {
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

        // Part 6: interactive line drawing
    uint32_t current_color = MFB_RGB((uint8_t)line_r,(uint8_t)line_g,(uint8_t)line_b);//color from sliders

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
    if (drawing &&!(ctx->mouse_down & MU_MOUSE_LEFT)) {
        lines.push_back({start_x,start_y,current_x,current_y,current_color});
        drawing = false;
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
// Draw temporary preview while dragging
if (drawing) {
    draw_line(
        start_x,
        start_y,
        current_x,
        current_y,
        current_color
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
    if (mu_begin_window(ctx, "Widgets", mu_rect(20, 20, 360, 700))) {
      int w1[] = {-1};

      // label / text
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "mu_label: plain static text");
      mu_text(ctx, "mu_text: word-wrapped longer text that will reflow inside "
                   "the window width automatically.");

      // button
      mu_layout_row(ctx, 1, w1, 0);
      if (mu_button(ctx, "mu_button: click me")) {
        quit_requested = false; // just a reaction
      }

      // checkbox
      mu_layout_row(ctx, 1, w1, 0);
      mu_checkbox(ctx, "mu_checkbox A (off)", &checkbox_a);
      mu_checkbox(ctx, "mu_checkbox B (on)", &checkbox_b);

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
    // --- Panel window ---
    if (mu_begin_window(ctx, "Panel Demo", mu_rect(395, 20, 380, 200))) {
      int w2[] = {-1};
      mu_layout_row(ctx, 1, w2, 120);
      mu_begin_panel(ctx, "scrollable panel");
      int wp[] = {-1};
      for (int i = 1; i <= 12; i++) {
        mu_layout_row(ctx, 1, wp, 0);
        char line[32];
        snprintf(line, sizeof(line), "Panel row %d", i);
        mu_label(ctx, line);
      }
      mu_end_panel(ctx);
      mu_end_window(ctx);
    }

    // --- Popup demo window ---
    if (mu_begin_window(ctx, "Popup Demo", mu_rect(395, 235, 380, 80))) {
      int w3[] = {-1};
      mu_layout_row(ctx, 1, w3, 0);
      if (mu_button(ctx, "Open popup")) {
        mu_Container *popup = mu_get_container(ctx, "my popup");
        popup->rect = mu_rect(ctx->mouse_pos.x, ctx->mouse_pos.y, 260, 84);
        popup->open = 1;
        ctx->hover_root = ctx->next_hover_root = popup;
        mu_bring_to_front(ctx, popup);
      }
      int popup_opt = MU_OPT_POPUP | MU_OPT_NORESIZE | MU_OPT_NOSCROLL |
                      MU_OPT_NOTITLE | MU_OPT_CLOSED;
      if (mu_begin_window_ex(ctx, "my popup", mu_rect(0, 0, 260, 84),
                             popup_opt)) {
        int wp[] = {-1};
        mu_layout_row(ctx, 1, wp, 0);
        mu_label(ctx, "mu_popup: click outside to close");
        if (mu_button(ctx, "Close")) {
          mu_get_current_container(ctx)->open = 0;
        }
        mu_end_window(ctx);
      }
      mu_end_window(ctx);
    }

    mu_end(ctx);

    if (quit_requested) {
      mfb_close(window);
      break;
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
