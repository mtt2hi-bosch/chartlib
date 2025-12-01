# ChartLib Usage Guide (with window title and improved chart title rendering)

## 1. Include the Header

```c
#include "chartlib.h"
```

## 2. Configure and Initialize

```c
chartlib_init_options_t opts = {0};
opts.chart_count = 2;
opts.columns_per_chart[0] = 4;
opts.columns_per_chart[1] = 3;

opts.style.bg_color = (chartlib_color_t){240,240,250};
opts.style.chart_title_color = (chartlib_color_t){50,50,200};
opts.style.column_label_color = (chartlib_color_t){0,0,0};
opts.style.value1_color = (chartlib_color_t){100,200,255};
opts.style.value2_color = (chartlib_color_t){220,220,220};
opts.style.border_color = (chartlib_color_t){90,90,90};

void on_event(chartlib_event_type_t event_type, void *userdata) {
    if (event_type == CHARTLIB_EVENT_CLOSE) {
        printf("ChartLib: window closed!\n");
        *(int*)userdata = 0;
    }
}
int running = 1;
opts.event_cb = on_event;
opts.event_cb_userdata = &running;

if (chartlib_init(&opts) != CHARTLIB_OK) {
    fprintf(stderr, "ChartLib: Initialization failed\n");
    exit(1);
}
```

## 3. Set Window Title

```c
chartlib_set_window_title("CPU Quality Charts");
```

## 4. Set Chart Titles, Labels, and Values

```c
chartlib_set_chart_title(0, "Quality CPU 1");
chartlib_set_chart_title(1, "Quality CPU 2");

chartlib_set_column_label(0, 0, "Core 1");
chartlib_set_column_values(0, 0, 35.0, 60.0);

chartlib_set_column_label(1, 2, "Turbo");
chartlib_set_column_values(1, 2, 40.0, 85.0);
```

## 5. Redraw

```c
chartlib_update();
```

## 6. Main Loop

```c
while (running) {
    // Update data for charts if needed
    chartlib_set_column_values(...);
    chartlib_update(); // update window

    sleep(1);
}
```

Window close sets `running=0` via the event callback.

## 7. Shutdown

```c
chartlib_close();
```

## 8. Improved Chart Title Placement

Chart titles are now drawn with extra top padding, appearing **above** the chart frame instead of overlapping it.

---

## Main Functions Recap

- `int chartlib_init(const chartlib_init_options_t *opts);`
- `void chartlib_close(void);`
- `int chartlib_set_window_title(const char *title);`          *(set window caption)*
- `int chartlib_set_chart_title(unsigned int chart_idx, const char *title);`
- `int chartlib_set_column_label(unsigned int chart_idx, unsigned int col_idx, const char *label);`
- `int chartlib_set_column_values(unsigned int chart_idx, unsigned int col_idx, float value1, float value2);`
- `int chartlib_update(void);`
- `int chartlib_set_style(const chartlib_style_t *style);`     *(runtime style change)*

---

## Build

```sh
gcc -o chartdemo main.c chartlib.c -lX11 -lpthread
```

Let me know if you need more features, a Makefile, or further demo samples!