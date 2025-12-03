#ifndef CHARTLIB_H
#define CHARTLIB_H

#include <stddef.h>
#include <stdint.h>

#define CHARTLIB_MAX_CHARTS    32
#define CHARTLIB_MAX_COLUMNS   128
#define CHARTLIB_MAX_LABEL_LEN 64
#define CHARTLIB_MAX_TITLE_LEN 64

typedef struct {
    uint8_t r, g, b;
} chartlib_color_t;

typedef struct {
    char      label[CHARTLIB_MAX_LABEL_LEN];
    float     value1; // Within custom range (default: 0.0 - 100.0)
    float     value2; // Within custom range (default: 0.0 - 100.0) (value1 <= value2)
} chartlib_column_t;

typedef enum {
    CHARTLIB_EVENT_NONE = 0,
    CHARTLIB_EVENT_CLOSE = 1,
} chartlib_event_type_t;

typedef void (*chartlib_event_callback_t)(chartlib_event_type_t event_type, void *user_data);

/**
 * Column label orientation options
 */
typedef enum {
    CHARTLIB_LABEL_HORIZONTAL = 0,        // Label text runs horizontally (left to right)
    CHARTLIB_LABEL_VERTICAL_BOTTOM_LEFT,  // Label text runs vertically, bottom to top, aligned to left edge
    CHARTLIB_LABEL_VERTICAL_BOTTOM_RIGHT, // Label text runs vertically, bottom to top, aligned to right edge
} chartlib_label_orientation_t;

/**
 * Column label box alignment relative to chart column
 */
typedef enum {
    CHARTLIB_LABEL_ALIGN_LEFT = 0,  // Label positioned to the left of the column
    CHARTLIB_LABEL_ALIGN_RIGHT,     // Label positioned to the right of the column
} chartlib_label_alignment_t;

/**
 * Column label display options
 * 
 * Example usage:
 * ```c
 * chartlib_column_label_opts_t label_opts = {
 *     .orientation = CHARTLIB_LABEL_VERTICAL_BOTTOM_LEFT,
 *     .alignment = CHARTLIB_LABEL_ALIGN_LEFT,
 *     .max_chars = 10  // Truncate labels longer than 10 characters
 * };
 * chartlib_set_column_label_options(chart_handle, &label_opts);
 * ```
 */
typedef struct {
    chartlib_label_orientation_t orientation;  // Label orientation (default: CHARTLIB_LABEL_HORIZONTAL)
    chartlib_label_alignment_t   alignment;    // Label alignment (default: CHARTLIB_LABEL_ALIGN_LEFT)
    unsigned int                 max_chars;    // Maximum characters to display (0 = no limit, default: 0)
} chartlib_column_label_opts_t;

typedef struct {
    chartlib_color_t bg_color;
    chartlib_color_t chart_title_color;
    chartlib_color_t column_label_color;
    chartlib_color_t value1_color;
    chartlib_color_t value2_color;
    chartlib_color_t border_color;
} chartlib_style_t;

typedef struct {
    unsigned int       chart_count; // 1-16
    unsigned int       columns_per_chart[CHARTLIB_MAX_CHARTS]; // #columns per chart
    chartlib_style_t   style;
    chartlib_event_callback_t event_cb;
    void            *event_cb_userdata;
} chartlib_init_options_t;

int chartlib_init(const chartlib_init_options_t *opts);
void chartlib_close(void);

int chartlib_set_window_title(const char *title);

int chartlib_set_chart_title(unsigned int chart_idx, const char *title);
int chartlib_set_column_label(unsigned int chart_idx, unsigned int col_idx, const char *label);
int chartlib_set_column_values(unsigned int chart_idx, unsigned int col_idx, float value1, float value2);

/**
 * Set column label display options for a specific chart
 * 
 * Controls how column labels are rendered for the specified chart, including:
 * - Label orientation (horizontal or vertical)
 * - Label alignment relative to the column (left or right)
 * - Maximum character count (truncates with "..." if exceeded)
 * 
 * @param chart_idx The chart index (0-based)
 * @param opts Pointer to label options struct. If NULL, resets to defaults.
 * @return CHARTLIB_OK on success, CHARTLIB_ERR_PARAM if chart_idx is invalid
 * 
 * Example:
 * ```c
 * chartlib_column_label_opts_t opts = {
 *     .orientation = CHARTLIB_LABEL_VERTICAL_BOTTOM_LEFT,
 *     .alignment = CHARTLIB_LABEL_ALIGN_RIGHT,
 *     .max_chars = 8
 * };
 * chartlib_set_column_label_options(0, &opts);
 * ```
 */
int chartlib_set_column_label_options(unsigned int chart_idx, const chartlib_column_label_opts_t *opts);

int chartlib_set_value_range(int min, int max);

int chartlib_update(void);

int chartlib_set_style(const chartlib_style_t *style);

#define CHARTLIB_OK            0
#define CHARTLIB_ERR_PARAM    -1
#define CHARTLIB_ERR_RANGE    -2
#define CHARTLIB_ERR_SYSTEM   -3

#endif
