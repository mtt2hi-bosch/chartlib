#include "chartlib.h"
#include <stdio.h>
#include <unistd.h>

void my_event_cb(chartlib_event_type_t t, void *u) {
    if (t == CHARTLIB_EVENT_CLOSE) *(int*)u = 0;
}

// Test program to verify all label options work correctly
int main() {
    chartlib_init_options_t opts = {0};
    opts.chart_count = 3;
    opts.columns_per_chart[0] = 3;
    opts.columns_per_chart[1] = 3;
    opts.columns_per_chart[2] = 3;
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
    chartlib_set_window_title("Label Options Test - Horizontal | Vertical Left | Vertical Right");

    // Chart 0: Horizontal labels with truncation
    chartlib_set_chart_title(0, "Horizontal (max 5 chars)");
    chartlib_column_label_opts_t horiz_opts = {
        .orientation = CHARTLIB_LABEL_HORIZONTAL,
        .alignment = CHARTLIB_LABEL_ALIGN_LEFT,
        .max_chars = 5
    };
    chartlib_set_column_label_options(0, &horiz_opts);
    chartlib_set_column_label(0, 0, "Short");
    chartlib_set_column_values(0, 0, 30, 70);
    chartlib_set_column_label(0, 1, "VeryLongName");
    chartlib_set_column_values(0, 1, 50, 80);
    chartlib_set_column_label(0, 2, "OK");
    chartlib_set_column_values(0, 2, 20, 60);

    // Chart 1: Vertical bottom-left labels, no truncation
    chartlib_set_chart_title(1, "Vertical Bottom-Left");
    chartlib_column_label_opts_t vert_left_opts = {
        .orientation = CHARTLIB_LABEL_VERTICAL_BOTTOM_LEFT,
        .alignment = CHARTLIB_LABEL_ALIGN_LEFT,
        .max_chars = 0
    };
    chartlib_set_column_label_options(1, &vert_left_opts);
    chartlib_set_column_label(1, 0, "CoreA");
    chartlib_set_column_values(1, 0, 40, 75);
    chartlib_set_column_label(1, 1, "CoreB");
    chartlib_set_column_values(1, 1, 55, 85);
    chartlib_set_column_label(1, 2, "HT1");
    chartlib_set_column_values(1, 2, 25, 50);

    // Chart 2: Vertical bottom-right labels with truncation
    chartlib_set_chart_title(2, "Vertical Bottom-Right (max 4)");
    chartlib_column_label_opts_t vert_right_opts = {
        .orientation = CHARTLIB_LABEL_VERTICAL_BOTTOM_RIGHT,
        .alignment = CHARTLIB_LABEL_ALIGN_LEFT,
        .max_chars = 4
    };
    chartlib_set_column_label_options(2, &vert_right_opts);
    chartlib_set_column_label(2, 0, "CPU");
    chartlib_set_column_values(2, 0, 35, 65);
    chartlib_set_column_label(2, 1, "Turbo");
    chartlib_set_column_values(2, 1, 60, 90);
    chartlib_set_column_label(2, 2, "Idle");
    chartlib_set_column_values(2, 2, 10, 40);

    chartlib_update();

    printf("Test initialized. Close window to exit.\n");
    printf("Chart 0: Horizontal labels with max_chars=5\n");
    printf("Chart 1: Vertical bottom-left labels, no limit\n");
    printf("Chart 2: Vertical bottom-right labels with max_chars=4\n");

    // Keep window open
    while (running) {
        sleep(1);
    }

    chartlib_close();
    printf("Test completed successfully!\n");
    return 0;
}
