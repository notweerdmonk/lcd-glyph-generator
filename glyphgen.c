
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <locale.h>

#define MAX_X 5
#define MAX_Y 10

#define C_X 3
#define C_Y 5
#define D_X 4
#define D_Y 2
#define N_X 5
#define N_Y 8

/* ncurses.h from libncursesw5 does not declare these fuctions */
extern int mvaddwstr(int, int, wchar_t*);
extern int addwstr(wchar_t*);

enum output_fmt {
  BIN,
  HEX,
};

static wchar_t dark_pixel[] = L"\u25A1";
static wchar_t lit_pixel[] = L"\u25A0";

static
void print_usage(const char *progname) {
  printf("Usage: %s -c <cols> -r <rows> -f <output_fmt>\n", progname);
  printf("    -c      number of columns\n"
         "    -r      number of rows\n"
         "    -f      output format, b - binary, h or anything  else - HEX\n"
         "    -h      print this message\n");
}

static
inline
int parse_options(int argc, char *argv[], int *n_y, int *n_x,
    enum output_fmt *fmt) {
  int opt;
  while ((opt = getopt(argc, argv, "+:r:c:f:h")) > 0) {
    switch(opt) {
      case 'r':
        *n_y = atoi(optarg);
        break;
      case 'c':
        *n_x = atoi(optarg);
        break;
      case 'f':
        if (optarg[0] == 'h')
          *fmt = HEX;
        else
          *fmt = BIN;
        break;
      case 'h':
      default:
        if(opt == ':') {
          fprintf(stderr, "Invalid option: -%c requires an argument\n", optopt);
        }
        else if (opt == '?') {
          fprintf(stderr, "Invalid option: -%c\n", optopt);
        }
        print_usage(argv[0]);
        return -1;
    }
  }

  if (*n_x < 1) *n_x = 1;
  if (*n_x > MAX_X) *n_x = MAX_X;
  if (*n_y < 1) *n_y = 1;
  if (*n_y > MAX_Y) *n_y = MAX_Y;

  return optind;
}

static
inline
void init_screen() {
  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  clear();
}

static
inline
void deinit_screen() {
  endwin();
}

static
inline
void output_glyph(const char (*p_glyph)[5], int n_y, int n_x,
    enum output_fmt fmt) {
  printf("Glyph (%d x %d)\n" \
         "Format: %s\n", n_x, n_y, (fmt == BIN) ? "bin" : "hex");
  for (int r = 0; r < n_y; r++) {
    int out = 0;
    char buffer[8] = { 0, };
    for (int c = 0; c < n_x; c++) {
      if (fmt == BIN)
        buffer[c] = (p_glyph[r][c] == '1') ? '1' : '0';
      else
        out += (p_glyph[r][c] == '1') ? 1 << (n_x - 1 - c) : 0;
    }
    if (fmt == BIN)
      printf("  0b%s\n", buffer);
    else
      printf("  0x%x\n", out);
  }
}

#define get_cursor(y, x) getyx(stdscr, y, x)

static
inline
void move_cursor(int y, int x) {
  move(y, x);
}

static
inline
void disp_str_at(int y, int x, const char *str) {
  mvaddstr(y, x, str);
}

static
inline
void disp_pixel(wchar_t* ch) {
  addwstr(ch);
}

static
inline
void disp_pixel_at(int y, int x, wchar_t* ch) {
  mvaddwstr(y, x, ch);
}

static
inline
void disp_row(int y, int x, int n, int d, wchar_t* ch) {
  for((d < 1) ? 1 : d; n > 0; n--, x += d)
    disp_pixel_at(y, x, ch);
}

static
inline __attribute__ ((unused))
void disp_col(int y, int x, int n, int d, wchar_t* ch) {
  for((d < 1) ? 1 : d; n > 0; n--, y += d)
    disp_pixel_at(y, x, ch);
}

static
inline
void disp_rect(int y, int x, int h, int w, int dy, int dx, wchar_t* ch) {
  for ((dy < 1) ? 1 : dy; h > 0; h--, y += dy) {
    disp_row(y, x, w, dx, ch);
  }
}

static
inline
void draw() {
  refresh();
}

int main(int argc, char *argv[]) {

  char glyph[MAX_Y][MAX_X] = { 0, };
  enum output_fmt fmt = BIN;
  int c_x = C_X;
  int c_y = C_Y;
  int n_x = N_X;
  int n_y = N_Y;
  int r = 0;
  int c = 0;
  int exit_cond = 0;

  if (parse_options(argc, argv, &n_y, &n_x, &fmt) < 0) {
    return 0;
  }

  init_screen();

  /* display banner, legend and pixels */
  disp_str_at(C_Y - 4, C_X, "LCD Glyph Generator");
  disp_str_at(C_Y - 2, C_X, "f:fill  d:delete  c:clear  q:exit");
  disp_rect(c_y, c_x, n_y, n_x, D_Y, D_X, dark_pixel);
  move_cursor(c_y, c_x);
  draw();

  /* this loop does most of the work */
  while (!exit_cond) {
    int c_y_prev = c_y;
    int c_x_prev = c_x;
    int ch = getch();
    switch (ch) {
      case 'k':
      case KEY_UP:
        if (c_y > C_Y)
            c_y -= D_Y, --c;
        break;
      case 'j':
      case KEY_DOWN:
        if (c_y < (C_Y + D_Y * (n_y - 1)))
          c_y += D_Y, ++c;
        break;
      case 'h':
      case KEY_LEFT:
        if (c_x > C_X)
          c_x -= D_X, --r;
        break;
      case 'l':
      case KEY_RIGHT:
        if (c_x < (C_X + D_X * (n_x - 1)))
          c_x += D_X, ++r;
        break;
      case 'f':
        disp_pixel(lit_pixel);
        get_cursor(c_y_prev, c_x_prev);
        glyph[c][r] = '1';
        break;
      case 'd':
        disp_pixel(dark_pixel);
        get_cursor(c_y_prev, c_x_prev);
        glyph[c][r] = '0';
        break;
      case 'c':
        c_y = C_Y, c_x = C_X;
        disp_rect(c_y, c_x, n_y, n_x, D_Y, D_X, dark_pixel);
        get_cursor(c_y_prev, c_x_prev);
        memset(glyph, 0, sizeof(glyph));
        r = c = 0;
        break;
      case 'q':
        exit_cond = 1;
      default:
        ;;
    }

    if (c_y != c_y_prev || c_x != c_x_prev) {
      move_cursor(c_y, c_x);
      draw();
    }
  }

  /* close curses screen for printing to terminal */
  deinit_screen();

  output_glyph(glyph, n_y, n_x, fmt);

  return 0;
}
