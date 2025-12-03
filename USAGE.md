# ChartLib Usage Guide (with Cairo-powered rotated labels)

## 1. Requirements

- Xlib (`libx11-dev`)
- Cairo graphics (`libcairo2-dev`)

Install on Ubuntu/Debian with:
```sh
sudo apt-get install libx11-dev libcairo2-dev
```

## 2. Include the Header

```c
#include "chartlib.h"
```

## 3. Configure and Initialize

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
    if (event_type == CHARTLIB_EVENT_CLOSE)
        *(int*)userdata = 0;
}
int running = 1;
opts.event_cb = on_event;
opts.event_cb_userdata = &running;

if (chartlib_init(&opts) != CHARTLIB_OK) {
    fprintf(stderr, "ChartLib: Initialization failed\n");
    exit(1);
}
```

## 4. Set Window Title

```c
chartlib_set_window_title("CPU Quality Charts");
```

## 5. Set Global Column Label Style (supports vertical alignment!)

```c
chartlib_set_column_label_style(
    CHARTLIB_LABEL_VERTICAL_BOTLEFT, // vertical, top-to-bottom
    CHARTLIB_LABEL_ALIGN_RIGHT,      // right edge alignment
    7                                // max label chars
);
```

## 6. Set Chart Titles, Labels, and Values

```c
chartlib_set_chart_title(0, "Quality CPU 1");
chartlib_set_chart_title(1, "Quality CPU 2");
chartlib_set_column_label(0, 0, "Core 1");
chartlib_set_column_values(0, 0, 35.0, 60.0);
```

## 7. Redraw

```c
chartlib_update();
```

## 8. Main Loop

```c
while (running) {
    chartlib_set_column_values(0, 0, ...); // update values
    chartlib_update();
    sleep(1);
}
```

## 9. Shutdown

```c
chartlib_close();
```

---

## Build

```sh
gcc -o chartdemo main.c chartlib.c -lX11 -lcairo -lpthread
```

---

## Notes

- **Vertical label orientations are now fully supported** using Cairo rotation.
- **All ChartLib API functions remain unchanged**. Simply link with Cairo.
- **Alignment and maximal label length** handled as requested.