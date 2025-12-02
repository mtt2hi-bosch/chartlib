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

int chartlib_set_value_range(int min, int max);

int chartlib_update(void);

int chartlib_set_style(const chartlib_style_t *style);

#define CHARTLIB_OK            0
#define CHARTLIB_ERR_PARAM    -1
#define CHARTLIB_ERR_RANGE    -2
#define CHARTLIB_ERR_SYSTEM   -3

#endif
