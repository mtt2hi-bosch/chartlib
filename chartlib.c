#include "chartlib.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h> // For usleep

typedef struct {
    char title[CHARTLIB_MAX_TITLE_LEN];
    chartlib_column_t columns[CHARTLIB_MAX_COLUMNS];
    unsigned int num_columns;
    chartlib_column_label_opts_t label_opts;
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

// Constants for label rendering
#define VERTICAL_CHAR_SPACING 12
#define VERTICAL_LABEL_OFFSET 8

static unsigned long color_pixel(chartlib_color_t c) {
    return ((c.r & 0xff) << 16) | ((c.g & 0xff) << 8) | (c.b & 0xff);
}

/**
 * Truncate or format label text based on max_chars setting
 * Returns a static buffer with the formatted label
 */
static const char* format_label(const char *label, unsigned int max_chars) {
    static char buffer[CHARTLIB_MAX_LABEL_LEN];
    
    if (max_chars == 0 || strlen(label) <= max_chars) {
        return label;
    }
    
    // Ensure max_chars doesn't exceed buffer size
    if (max_chars >= CHARTLIB_MAX_LABEL_LEN) {
        max_chars = CHARTLIB_MAX_LABEL_LEN - 1;
    }
    
    // Truncate with ellipsis
    if (max_chars <= 3) {
        strncpy(buffer, label, max_chars);
        buffer[max_chars] = '\0';
    } else {
        strncpy(buffer, label, max_chars - 3);
        buffer[max_chars - 3] = '\0';
        strcat(buffer, "...");
    }
    
    return buffer;
}

/**
 * Draw column label with specified orientation and alignment
 */
static void draw_column_label(const char *label, int cx, int col_y, int col_w, int bh,
                              const chartlib_column_label_opts_t *opts) {
    const char *display_label = format_label(label, opts->max_chars);
    int label_len = strlen(display_label);
    
    if (label_len == 0) return;
    
    if (opts->orientation == CHARTLIB_LABEL_HORIZONTAL) {
        // Horizontal label
        int label_x = cx;
        if (opts->alignment == CHARTLIB_LABEL_ALIGN_RIGHT) {
            label_x = cx + col_w - 4; // Right side of column
        }
        XDrawString(dpy, win, gc, label_x, col_y + bh + 16, display_label, label_len);
        
    } else {
        // Vertical labels (both orientations)
        int label_x;
        
        // Determine base X position based on orientation
        if (opts->orientation == CHARTLIB_LABEL_VERTICAL_BOTTOM_LEFT) {
            label_x = cx;
        } else { // CHARTLIB_LABEL_VERTICAL_BOTTOM_RIGHT
            label_x = cx + VERTICAL_LABEL_OFFSET;
        }
        
        // Apply alignment override if specified
        if (opts->alignment == CHARTLIB_LABEL_ALIGN_RIGHT) {
            label_x = cx + col_w - 4;
        }
        
        int start_y = col_y + bh + 16;
        
        // Draw each character vertically (one below the next)
        for (int i = 0; i < label_len; ++i) {
            char ch[2] = {display_label[i], '\0'};
            XDrawString(dpy, win, gc, label_x, start_y + i * VERTICAL_CHAR_SPACING, ch, 1);
        }
    }
}

static void draw_charts(void) {
    XClearWindow(dpy, win);

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

        // Vertical padding for title
        int title_pad = 22; // px
        int border_pad = 4; // px

        // Draw chart title (above frame, so NOT overlapped)
        XSetForeground(dpy, gc, color_pixel(style.chart_title_color));
        XDrawString(dpy, win, gc, x0 + 16, y0 + title_pad, charts[idx].title, strlen(charts[idx].title));

        // Draw chart border, shifted down for title space
        XSetForeground(dpy, gc, color_pixel(style.border_color));
        XDrawRectangle(dpy, win, gc,
            x0 + border_pad, y0 + title_pad,
            chart_w - 2*border_pad, chart_h - title_pad - border_pad);

        // Draw Y-axis labels (min and max)
        XSetForeground(dpy, gc, color_pixel(style.column_label_color));
        char max_label[32], min_label[32];
        snprintf(max_label, sizeof(max_label), "%d", value_range_max);
        snprintf(min_label, sizeof(min_label), "%d", value_range_min);
        int y_axis_x = x0 + border_pad + 4;
        int y_axis_top = y0 + title_pad + 16;
        int y_axis_bottom = y0 + chart_h - border_pad - 18;
        XDrawString(dpy, win, gc, y_axis_x, y_axis_top + 10, max_label, strlen(max_label));
        XDrawString(dpy, win, gc, y_axis_x, y_axis_bottom, min_label, strlen(min_label));

        // Draw columns
        unsigned int ncol = charts[idx].num_columns;
        if (ncol == 0) continue;

        int col_w = (chart_w - 2*border_pad - 20) / ncol;
        int col_max_h = chart_h - 2*border_pad - title_pad - 34;
        int col_y = y0 + title_pad + 16;

        // Calculate range span for normalization
        float range_span = (float)(value_range_max - value_range_min);
        if (range_span == 0.0f) range_span = 1.0f; // Avoid division by zero

        for (unsigned int c = 0; c < ncol; ++c) {
            chartlib_column_t *col = &charts[idx].columns[c];
            int cx = x0 + border_pad + 10 + c * col_w;
            int bh = col_max_h;
            // Normalize values based on custom range
            float norm_v2 = (col->value2 - value_range_min) / range_span;
            float norm_v1 = (col->value1 - value_range_min) / range_span;
            int v2_h = (int)(norm_v2 * bh);
            int v1_h = (int)(norm_v1 * bh);

            // value2 bar (background)
            XSetForeground(dpy, gc, color_pixel(style.value2_color));
            XFillRectangle(dpy, win, gc, cx, col_y + bh - v2_h, col_w - 4, v2_h);

            // value1 bar (foreground)
            XSetForeground(dpy, gc, color_pixel(style.value1_color));
            XFillRectangle(dpy, win, gc, cx, col_y + bh - v1_h, col_w - 4, v1_h);

            // borders around column
            XSetForeground(dpy, gc, color_pixel(style.border_color));
            XDrawRectangle(dpy, win, gc, cx, col_y, col_w - 4, bh);

            // Draw label with custom options
            XSetForeground(dpy, gc, color_pixel(style.column_label_color));
            draw_column_label(col->label, cx, col_y, col_w, bh, &charts[idx].label_opts);
        }
    }
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
        // Initialize default label options
        charts[i].label_opts.orientation = CHARTLIB_LABEL_HORIZONTAL;
        charts[i].label_opts.alignment = CHARTLIB_LABEL_ALIGN_LEFT;
        charts[i].label_opts.max_chars = 0; // No limit by default
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
    XStoreName(dpy, win, window_title); // initial window title
    XSetIconName(dpy, win, window_title);

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
    
    // Clamp values to the configured range
    if (value1 < value_range_min) value1 = value_range_min;
    if (value1 > value_range_max) value1 = value_range_max;
    if (value2 < value_range_min) value2 = value_range_min;
    if (value2 > value_range_max) value2 = value_range_max;
    
    // After clamping, ensure value1 <= value2
    if (value1 > value2) value1 = value2;
    
    charts[chart_idx].columns[col_idx].value1 = value1;
    charts[chart_idx].columns[col_idx].value2 = value2;
    return CHARTLIB_OK;
}

int chartlib_set_column_label_options(unsigned int chart_idx, const chartlib_column_label_opts_t *opts) {
    if (chart_idx >= chart_count) return CHARTLIB_ERR_PARAM;
    
    if (opts) {
        charts[chart_idx].label_opts = *opts;
    } else {
        // Reset to defaults
        charts[chart_idx].label_opts.orientation = CHARTLIB_LABEL_HORIZONTAL;
        charts[chart_idx].label_opts.alignment = CHARTLIB_LABEL_ALIGN_LEFT;
        charts[chart_idx].label_opts.max_chars = 0;
    }
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
