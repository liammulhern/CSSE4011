#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>

#include "display/lv_display.h"

#include <lvgl.h>

/** Number of rows in the grid. */
#define GRID_ROWS      40
/** Number of columns in the grid. */
#define GRID_COLS      30
/** Pixel size of each grid cell. */
#define CELL_SIZE      20
/** X offset of grid from the left edge of the screen. */
#define GRID_OFFSET_X  0
/** Y offset of grid from the top of the screen. */
#define GRID_OFFSET_Y  0
/** Color for axis lines and labels. */
#define AXIS_COLOR    lv_color_make(255, 255, 255)
/** Major axis line width. */
#define AXIS_WIDTH     1

/** Color of the marker (LVGL color). */
#define MARKER_COLOR lv_color_make(255, 0, 0)

/** LVGL chart object representing the scatter grid. */
static lv_obj_t *chart;
/** Series handle for the marker point. */
static lv_chart_series_t *marker_ser;

LOG_MODULE_REGISTER(gui_module, LOG_LEVEL_INF);

/**
 * @brief Create an 8×8 scatter-mode chart as the grid.
 * @param parent The parent LVGL object (e.g., current screen).
 *
 * Sets up a chart sized GRID_COLS×CELL_SIZE by GRID_ROWS×CELL_SIZE,
 * enables scatter mode, draws division lines for the grid, and
 * adds a single-point series for the marker.
 */
static void gui_create_scatter_grid(lv_obj_t *parent)
{
    /* 1) Chart setup */
    chart = lv_chart_create(parent);
    lv_obj_set_size(chart, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(chart, GRID_OFFSET_X, GRID_OFFSET_Y);
    lv_chart_set_type(chart, LV_CHART_TYPE_SCATTER);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_CIRCULAR);

    lv_chart_set_axis_range(chart, LV_CHART_AXIS_PRIMARY_X, 0, GRID_COLS);
    lv_chart_set_axis_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, GRID_ROWS);

    lv_chart_set_point_count(chart, 2);

    lv_chart_set_div_line_count(chart, GRID_COLS / 5, GRID_ROWS / 5);

    marker_ser = lv_chart_add_series(chart, MARKER_COLOR, LV_CHART_AXIS_PRIMARY_Y);
}

/**
 * @brief Plot or move the marker at given grid coordinates.
 * @param col Column index (0-based) within the 8×8 grid.
 * @param row Row index (0-based) within the 8×8 grid.
 *
 * Internally, sets the marker point to (col+0.5, row+0.5) so that
 * the circle is centered in the grid cell.
 */
void gui_draw_localisation_position(double col, double row) { 
    lv_chart_set_next_value2(chart, marker_ser, row*10, col*10);
    lv_chart_refresh(chart);
}

int gui_init() {
    const struct device *display_dev;

    display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

    if (!device_is_ready(display_dev)) {
        LOG_ERR("Device not ready, aborting test");
        return -ENODEV;
    }

    lv_obj_t *scr = lv_screen_active();
    gui_create_scatter_grid(scr);

    lv_timer_handler();
    display_blanking_off(display_dev);

    return 0;
}

void gui_thread(void) {
    while (1) {
        uint32_t sleep_ms = lv_timer_handler();

        k_msleep(MIN(sleep_ms, INT32_MAX));
    }
}
