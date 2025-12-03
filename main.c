#include "chartlib.h"
#include <stdio.h>
#include <unistd.h>

void my_event_cb(chartlib_event_type_t t, void *u) {
    if (t == CHARTLIB_EVENT_CLOSE) *(int*)u = 0;
}

// Example main: create two charts, each with columns, update, set title
int main() {
    chartlib_init_options_t opts = {0};
    opts.chart_count = 2;
    opts.columns_per_chart[0] = 4;
    opts.columns_per_chart[1] = 3;
    opts.style.bg_color = (chartlib_color_t){240,240,255};
    opts.style.chart_title_color = (chartlib_color_t){0,0,200};
    opts.style.column_label_color = (chartlib_color_t){0,0,0};
    opts.style.value1_color = (chartlib_color_t){0,200,0};
    opts.style.value2_color = (chartlib_color_t){200,200,200};
    opts.style.border_color = (chartlib_color_t){80,80,80};

    int running = 1;
    opts.event_cb = my_event_cb;
    opts.event_cb_userdata = &running;

    if (chartlib_init(&opts) != CHARTLIB_OK) {
        fprintf(stderr, "ChartLib: Initialization failed\n");
        return 1;
    }
    chartlib_set_window_title("CPU Quality Charts");

    chartlib_set_chart_title(0, "Quality CPU 1 (Horizontal Labels)");
    chartlib_set_chart_title(1, "Quality CPU 2 (Vertical Labels)");

    // Chart 0: Use horizontal labels with max 6 characters
    chartlib_column_label_opts_t horiz_opts = {
        .orientation = CHARTLIB_LABEL_HORIZONTAL,
        .alignment = CHARTLIB_LABEL_ALIGN_LEFT,
        .max_chars = 6  // Truncate labels longer than 6 chars
    };
    chartlib_set_column_label_options(0, &horiz_opts);

    chartlib_set_column_label(0, 0, "core A");
    chartlib_set_column_values(0, 0, 40, 60);

    chartlib_set_column_label(0, 1, "core B long name");  // Will be truncated
    chartlib_set_column_values(0, 1, 75, 89);

    chartlib_set_column_label(0, 2, "HT 1");
    chartlib_set_column_values(0, 2, 10, 30);

    chartlib_set_column_label(0, 3, "HT 2");
    chartlib_set_column_values(0, 3, 21, 62);

    // Chart 1: Use vertical labels with no character limit
    chartlib_column_label_opts_t vert_opts = {
        .orientation = CHARTLIB_LABEL_VERTICAL_BOTTOM_LEFT,
        .alignment = CHARTLIB_LABEL_ALIGN_LEFT,
        .max_chars = 0  // No limit
    };
    chartlib_set_column_label_options(1, &vert_opts);

    chartlib_set_column_label(1, 0, "core Z");
    chartlib_set_column_values(1, 0, 55, 80);

    chartlib_set_column_label(1, 1, "turbo");
    chartlib_set_column_values(1, 1, 48, 70);

    chartlib_set_column_label(1, 2, "E-core");
    chartlib_set_column_values(1, 2, 17, 92);

    chartlib_update();

    // Demo: loop and randomly update (simulate changing values)
    int tick = 0;
    while (running) {
        // Example: animate bars
        chartlib_set_column_values(0, 0, 40 + (tick % 50), 60 + (tick % 30));
        chartlib_update();
        sleep(1);
        tick++;
    }

    chartlib_close();
    return 0;
}