#include "chartlib.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <math.h>

typedef struct {
    char title[CHARTLIB_MAX_TITLE_LEN];
    chartlib_column_t columns[CHARTLIB_MAX_COLUMNS];
    unsigned int num_columns;
} chart_data_t;

static Display *dpy = NULL;
static int screen;
static Window win;
static GC gc;
static int win_w = 800, win_h = 600;
static pthread_t event_thread;
static int running = 0;
static int redraw_pending = 0;
static char window_title[128] = "ChartLib Charts";

static chart_data_t charts[CHARTLIB_MAX_CHARTS];
static unsigned int chart_count = 0;
static chartlib_style_t style;
static chartlib_event_callback_t event_cb = NULL;
static void *event_cb_userdata = NULL;
static int value_range_min = 0;
static int value_range_max = 100;

static cairo_surface_t *cr_surface = NULL;
static cairo_t *cr = NULL;

static chartlib_label_orientation_t g_label_orientation = CHARTLIB_LABEL_HORIZONTAL;
static chartlib_label_align_t      g_label_align = CHARTLIB_LABEL_ALIGN_LEFT;
static unsigned int                g_label_max_chars = CHARTLIB_MAX_LABEL_LEN;

void chartlib_set_column_label_style(
    chartlib_label_orientation_t orientation,
    chartlib_label_align_t align,
    unsigned int max_label_chars
) {
    g_label_orientation = orientation;
    g_label_align      = align;
    g_label_max_chars  = (max_label_chars > 0 && max_label_chars <= CHARTLIB_MAX_LABEL_LEN)
                          ? max_label_chars : CHARTLIB_MAX_LABEL_LEN;
}

static unsigned long color_pixel(chartlib_color_t c) {
    return ((c.r & 0xff) << 16) | ((c.g & 0xff) << 8) | (c.b & 0xff);
}

static void cairo_set_color(cairo_t *ctx, chartlib_color_t col) {
    cairo_set_source_rgb(ctx, col.r / 255.0, col.g / 255.0, col.b / 255.0);
}

static void cairo_draw_label(int x, int y, const char* label, chartlib_label_orientation_t orientation, chartlib_label_align_t align, chartlib_color_t color) {
    char clipped_label[CHARTLIB_MAX_LABEL_LEN + 1];
    size_t labellen = strlen(label);
    if (labellen > g_label_max_chars) {
        memcpy(clipped_label, label, g_label_max_chars);
        clipped_label[g_label_max_chars] = '\0';
        label = clipped_label;
        labellen = g_label_max_chars;
    } else {
        strcpy(clipped_label, label);
        label = clipped_label;
    }

    cairo_save(cr);
    cairo_set_color(cr, color);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 13);

    cairo_text_extents_t ext;
    cairo_text_extents(cr, label, &ext);

    double dx = 0, dy = 0;
    double angle = 0.0;
    switch (orientation) {
        case CHARTLIB_LABEL_HORIZONTAL:
            if (align == CHARTLIB_LABEL_ALIGN_LEFT)
                dx = 0;
            else
                dx = -ext.width;
            dy = 0;
            angle = 0.0;
            break;
        case CHARTLIB_LABEL_VERTICAL_BOTLEFT:
            if (align == CHARTLIB_LABEL_ALIGN_LEFT) {
                dx = 0;
                dy = 0;
            } else {
                dx = 0;
                dy = -ext.width;
            }
            angle = M_PI / 2;
            break;
        case CHARTLIB_LABEL_VERTICAL_BOTRIGHT:
            if (align == CHARTLIB_LABEL_ALIGN_LEFT) {
                dx = 0;
                dy = 0;
            } else {
                dx = 0;
                dy = ext.width;
            }
            angle = -M_PI / 2;
            break;
    }

    cairo_translate(cr, x, y);
    cairo_rotate(cr, angle);
    cairo_move_to(cr, dx, dy);
    cairo_show_text(cr, label);
    cairo_restore(cr);
}

static void draw_charts(void) {
    if (cr_surface) {
        cairo_xlib_surface_set_size(cr_surface, win_w, win_h);
    }
    cairo_save(cr);
    cairo_set_color(cr, style.bg_color);
    cairo_rectangle(cr, 0, 0, win_w, win_h);
    cairo_fill(cr);
    cairo_restore(cr);

    int cols = 1, rows = 1;
    while (cols * rows < chart_count) {
        if (cols == rows) cols++;
        else rows++;
    }
    int chart_w = win_w / cols;
    int chart_h = win_h / rows;

    for (unsigned int idx = 0; idx < chart_count; ++idx) {
        int chart_row = idx / cols;
        int chart_col = idx % cols;
        int x0 = chart_col * chart_w;
        int y0 = chart_row * chart_h;

        int title_pad = 22;
        int border_pad = 4;

        XSetForeground(dpy, gc, color_pixel(style.border_color));
        XDrawRectangle(dpy, win, gc,
            x0 + border_pad, y0 + title_pad,
            chart_w - 2*border_pad, chart_h - title_pad - border_pad);

        cairo_draw_label(x0 + 16, y0 + title_pad, charts[idx].title,
                        CHARTLIB_LABEL_HORIZONTAL, CHARTLIB_LABEL_ALIGN_LEFT, style.chart_title_color);

        char max_label[32], min_label[32];
        snprintf(max_label, sizeof(max_label), "%d", value_range_max);
        snprintf(min_label, sizeof(min_label), "%d", value_range_min);
        int y_axis_x = x0 + border_pad + 4;
        int y_axis_top = y0 + title_pad + 16;
        int y_axis_bottom = y0 + chart_h - border_pad - 18;
        cairo_draw_label(y_axis_x, y_axis_top + 10, max_label,
                        CHARTLIB_LABEL_HORIZONTAL, CHARTLIB_LABEL_ALIGN_LEFT, style.column_label_color);
        cairo_draw_label(y_axis_x, y_axis_bottom, min_label,
                        CHARTLIB_LABEL_HORIZONTAL, CHARTLIB_LABEL_ALIGN_LEFT, style.column_label_color);

        unsigned int ncol = charts[idx].num_columns;
        if (ncol == 0) continue;

        int col_w = (chart_w - 2*border_pad - 20) / ncol;

        int vertical_label_margin = 16; // Default for horizontal
        if (g_label_orientation != CHARTLIB_LABEL_HORIZONTAL) {
            cairo_save(cr);
            cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
            cairo_set_font_size(cr, 13);
            cairo_font_extents_t font_ext;
            cairo_font_extents(cr, &font_ext);
            vertical_label_margin = (int)(g_label_max_chars * font_ext.height) + 10;
            cairo_restore(cr);
        }

        int col_max_h = chart_h - 2*border_pad - title_pad - 34 - vertical_label_margin;
        int col_y = y0 + title_pad + 16;

        float range_span = (float)(value_range_max - value_range_min);
        if (range_span == 0.0f) range_span = 1.0f;

        for (unsigned int c = 0; c < ncol; ++c) {
            chartlib_column_t *col = &charts[idx].columns[c];
            int cx = x0 + border_pad + 10 + c * col_w;
            int bh = col_max_h;

            float norm_v2 = (col->value2 - value_range_min) / range_span;
            float norm_v1 = (col->value1 - value_range_min) / range_span;
            int v2_h = (int)(norm_v2 * bh);
            int v1_h = (int)(norm_v1 * bh);

            XSetForeground(dpy, gc, color_pixel(style.value2_color));
            XFillRectangle(dpy, win, gc, cx, col_y + bh - v2_h, col_w - 4, v2_h);

            XSetForeground(dpy, gc, color_pixel(style.value1_color));
            XFillRectangle(dpy, win, gc, cx, col_y + bh - v1_h, col_w - 4, v1_h);

            XSetForeground(dpy, gc, color_pixel(style.border_color));
            XDrawRectangle(dpy, win, gc, cx, col_y, col_w - 4, bh);

            int lx = cx, ly = col_y + bh + vertical_label_margin - 4; // default horizontal
            int gap = 6; // Space in pixels between bar base and first letter
            switch (g_label_orientation) {
                case CHARTLIB_LABEL_HORIZONTAL:
                    if (g_label_align == CHARTLIB_LABEL_ALIGN_LEFT)
                        lx = cx;
                    else
                        lx = cx + col_w - 4;
                    ly = col_y + bh + 16;
                    break;
                case CHARTLIB_LABEL_VERTICAL_BOTLEFT:
                    lx = cx + (col_w / 2);
                    ly = col_y + bh + gap; // Just below the bar area
                    break;
                case CHARTLIB_LABEL_VERTICAL_BOTRIGHT:
                    lx = cx + (col_w / 2);
                    ly = col_y + bh + vertical_label_margin - 4;
                    break;
            }
            cairo_draw_label(lx, ly, col->label, g_label_orientation, g_label_align, style.column_label_color);
        }
    }
    cairo_surface_flush(cr_surface);
    XFlush(dpy);
}

static void *event_thread_func(void *unused) {
    XEvent ev;
    while (running) {
        while (XPending(dpy)) {
            XNextEvent(dpy, &ev);
            if (ev.type == Expose) {
                draw_charts();
            } else if (ev.type == ConfigureNotify) {
                win_w = ev.xconfigure.width;
                win_h = ev.xconfigure.height;
                draw_charts();
            } else if (ev.type == ClientMessage) {
                running = 0;
                if (event_cb) event_cb(CHARTLIB_EVENT_CLOSE, event_cb_userdata);
            }
        }
        if (redraw_pending) {
            draw_charts();
            redraw_pending = 0;
        }
    }
    return NULL;
}

int chartlib_init(const chartlib_init_options_t *opts) {
    if (!opts || opts->chart_count < 1 || opts->chart_count > CHARTLIB_MAX_CHARTS)
        return CHARTLIB_ERR_RANGE;
    chart_count = opts->chart_count;
    for (unsigned int i = 0; i < chart_count; ++i) {
        charts[i].num_columns = opts->columns_per_chart[i];
        if (charts[i].num_columns < 1 || charts[i].num_columns > CHARTLIB_MAX_COLUMNS)
            return CHARTLIB_ERR_RANGE;
        snprintf(charts[i].title, CHARTLIB_MAX_TITLE_LEN, "Quality CPU %u", i+1);
        for (unsigned int c = 0; c < charts[i].num_columns; ++c) {
            charts[i].columns[c].label[0] = 0;
            charts[i].columns[c].value1 = 0.0f;
            charts[i].columns[c].value2 = 0.0f;
        }
    }
    style = opts->style;
    event_cb = opts->event_cb;
    event_cb_userdata = opts->event_cb_userdata;

    XInitThreads(); 
    dpy = XOpenDisplay(NULL);
    if (!dpy) return CHARTLIB_ERR_SYSTEM;
    screen = DefaultScreen(dpy);
    win = XCreateSimpleWindow(dpy, RootWindow(dpy, screen), 100, 100,
                             win_w, win_h, 1,
                             BlackPixel(dpy, screen),
                             color_pixel(style.bg_color));
    XSelectInput(dpy, win, ExposureMask|KeyPressMask|StructureNotifyMask);
    Atom wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, win, &wm_delete, 1);

    gc = XCreateGC(dpy, win, 0, NULL);

    XMapWindow(dpy, win);
    XStoreName(dpy, win, window_title);
    XSetIconName(dpy, win, window_title);

    cr_surface = cairo_xlib_surface_create(dpy, win, DefaultVisual(dpy, screen), win_w, win_h);
    cr = cairo_create(cr_surface);

    running = 1;
    if (pthread_create(&event_thread, NULL, event_thread_func, NULL))
        return CHARTLIB_ERR_SYSTEM;
    return CHARTLIB_OK;
}

int chartlib_set_window_title(const char *title) {
    if (!dpy || !win || !title) return CHARTLIB_ERR_PARAM;
    snprintf(window_title, sizeof(window_title), "%s", title);
    XStoreName(dpy, win, window_title);
    XSetIconName(dpy, win, window_title);
    XFlush(dpy);
    return CHARTLIB_OK;
}

void chartlib_close(void) {
    running = 0;
    pthread_join(event_thread, NULL);
    cairo_destroy(cr);
    cairo_surface_destroy(cr_surface);
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
    dpy = NULL;
}

int chartlib_set_chart_title(unsigned int chart_idx, const char *title) {
    if (chart_idx >= chart_count || !title) return CHARTLIB_ERR_PARAM;
    snprintf(charts[chart_idx].title, CHARTLIB_MAX_TITLE_LEN, "%s", title);
    return CHARTLIB_OK;
}

int chartlib_set_column_label(unsigned int chart_idx, unsigned int col_idx, const char *label) {
    if (chart_idx >= chart_count || col_idx >= charts[chart_idx].num_columns || !label) return CHARTLIB_ERR_PARAM;
    snprintf(charts[chart_idx].columns[col_idx].label, CHARTLIB_MAX_LABEL_LEN, "%s", label);
    return CHARTLIB_OK;
}

int chartlib_set_column_values(unsigned int chart_idx, unsigned int col_idx, float value1, float value2) {
    if (chart_idx >= chart_count || col_idx >= charts[chart_idx].num_columns)
        return CHARTLIB_ERR_PARAM;
    if (value1 < value_range_min) value1 = value_range_min;
    if (value1 > value_range_max) value1 = value_range_max;
    if (value2 < value_range_min) value2 = value_range_min;
    if (value2 > value_range_max) value2 = value_range_max;
    if (value1 > value2) value1 = value2;

    charts[chart_idx].columns[col_idx].value1 = value1;
    charts[chart_idx].columns[col_idx].value2 = value2;
    return CHARTLIB_OK;
}

int chartlib_set_value_range(int min, int max) {
    if (min >= max) return CHARTLIB_ERR_PARAM;
    value_range_min = min;
    value_range_max = max;
    return CHARTLIB_OK;
}

int chartlib_update(void) {
    redraw_pending = 1;
    return CHARTLIB_OK;
}

int chartlib_set_style(const chartlib_style_t *s) {
    if (!s) return CHARTLIB_ERR_PARAM;
    style = *s;
    redraw_pending = 1;
    return CHARTLIB_OK;
}