#include "page_imu.h"
#include "ui_manager.h"
#include "ui_utils.h"
#include "ui_style.h"
#include "ui_data.h"
#include "lvgl.h"
#include "esp_log.h"
#include <stdio.h>
#include <math.h>

static const char *TAG = "page_imu";

static ui_page_t s_page_imu = {
    .name    = "IMU",
    .screen  = NULL,
    .create  = NULL,
    .destroy = NULL,
    .update  = NULL,
};

#define CUBE_SIZE  100
#define CUBE_HALF  0.5f

static lv_obj_t *s_canvas;
static lv_color_t s_canvas_buf[CUBE_SIZE * CUBE_SIZE];
static lv_obj_t *s_label_acc_x;
static lv_obj_t *s_label_acc_y;
static lv_obj_t *s_label_acc_z;
static lv_obj_t *s_label_gyr_x;
static lv_obj_t *s_label_gyr_y;
static lv_obj_t *s_label_gyr_z;
static lv_obj_t *s_label_steps;
static lv_timer_t *s_imu_timer;

static float s_yaw = 0.0f;

typedef struct {
    float x, y, z;
} vec3_t;

static const vec3_t s_cube_v[8] = {
    {-CUBE_HALF, -CUBE_HALF, -CUBE_HALF},
    { CUBE_HALF, -CUBE_HALF, -CUBE_HALF},
    { CUBE_HALF,  CUBE_HALF, -CUBE_HALF},
    {-CUBE_HALF,  CUBE_HALF, -CUBE_HALF},
    {-CUBE_HALF, -CUBE_HALF,  CUBE_HALF},
    { CUBE_HALF, -CUBE_HALF,  CUBE_HALF},
    { CUBE_HALF,  CUBE_HALF,  CUBE_HALF},
    {-CUBE_HALF,  CUBE_HALF,  CUBE_HALF},
};

static const int s_cube_e[12][2] = {
    {0,1},{1,2},{2,3},{3,0},
    {4,5},{5,6},{6,7},{7,4},
    {0,4},{1,5},{2,6},{3,7},
};

static void rotate_vec(const vec3_t *in, vec3_t *out,
                        float roll, float pitch, float yaw)
{
    float cr = cosf(roll),  sr = sinf(roll);
    float cp = cosf(pitch), sp = sinf(pitch);
    float cy = cosf(yaw),   sy = sinf(yaw);

    float x = in->x, y = in->y, z = in->z;

    float x1 = x * cp + z * sp;
    float z1 = -x * sp + z * cp;
    float y1 = y;

    float y2 = y1 * cr - z1 * sr;
    float z2 = y1 * sr + z1 * cr;
    float x2 = x1;

    out->x = x2 * cy - y2 * sy;
    out->y = x2 * sy + y2 * cy;
    out->z = z2;
}

static void project(const vec3_t *v, int *sx, int *sy)
{
    float dist = 2.5f;
    float scale = dist / (dist + v->z);
    *sx = (int)(v->x * scale * (CUBE_SIZE * 0.38f) + CUBE_SIZE / 2);
    *sy = (int)(-v->y * scale * (CUBE_SIZE * 0.38f) + CUBE_SIZE / 2);
}

static void draw_cube(lv_obj_t *canvas, float roll, float pitch, float yaw)
{
    lv_canvas_fill_bg(canvas, lv_color_hex(0x0A0A1A), LV_OPA_COVER);

    vec3_t rotated[8];
    int projected[8][2];

    for (int i = 0; i < 8; i++) {
        rotate_vec(&s_cube_v[i], &rotated[i], roll, pitch, yaw);
        project(&rotated[i], &projected[i][0], &projected[i][1]);
    }

    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);

    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = lv_color_hex(0x4FC3F7);
    line_dsc.width = 2;
    line_dsc.round_start = 1;
    line_dsc.round_end = 1;

    for (int i = 0; i < 12; i++) {
        int a = s_cube_e[i][0];
        int b = s_cube_e[i][1];
        line_dsc.p1.x = projected[a][0];
        line_dsc.p1.y = projected[a][1];
        line_dsc.p2.x = projected[b][0];
        line_dsc.p2.y = projected[b][1];
        lv_draw_line(&layer, &line_dsc);
    }

    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_hex(0x4FC3F7);
    rect_dsc.bg_opa = LV_OPA_COVER;
    rect_dsc.radius = 2;

    for (int i = 0; i < 8; i++) {
        lv_area_t area = {
            .x1 = projected[i][0] - 2,
            .y1 = projected[i][1] - 2,
            .x2 = projected[i][0] + 2,
            .y2 = projected[i][1] + 2,
        };
        lv_draw_rect(&layer, &rect_dsc, &area);
    }

    lv_canvas_finish_layer(canvas, &layer);
}

static lv_obj_t *create_section_title(lv_obj_t *parent, int32_t y,
                                       const char *text, lv_color_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_pos(label, 0, y);
    lv_obj_set_align(label, LV_ALIGN_TOP_MID);
    return label;
}

static lv_obj_t *create_data_row(lv_obj_t *parent, int32_t y,
                                  const char *prefix, lv_color_t color)
{
    lv_obj_t *pre = lv_label_create(parent);
    lv_label_set_text(pre, prefix);
    lv_obj_set_style_text_font(pre, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(pre, color, 0);
    lv_obj_set_pos(pre, -ROUND_SAFE_W / 2 + 4, y);
    lv_obj_set_align(pre, LV_ALIGN_TOP_MID);

    lv_obj_t *val = lv_label_create(parent);
    lv_label_set_text(val, "0.00");
    lv_obj_set_style_text_font(val, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(val, lv_color_white(), 0);
    lv_obj_set_pos(val, -ROUND_SAFE_W / 2 + 30, y - 2);
    lv_obj_set_align(val, LV_ALIGN_TOP_MID);

    return val;
}

static void imu_timer_cb(lv_timer_t *timer)
{
    ui_sensor_data_t s = ui_get_sensor();

    if (s.imu_ready) {
        lv_label_set_text_fmt(s_label_acc_x, "%.2f", s.acc_x);
        lv_label_set_text_fmt(s_label_acc_y, "%.2f", s.acc_y);
        lv_label_set_text_fmt(s_label_acc_z, "%.2f", s.acc_z);
        lv_label_set_text_fmt(s_label_gyr_x, "%.1f", s.gyro_x);
        lv_label_set_text_fmt(s_label_gyr_y, "%.1f", s.gyro_y);
        lv_label_set_text_fmt(s_label_gyr_z, "%.1f", s.gyro_z);

        float pitch = atan2f(s.acc_x, sqrtf(s.acc_y * s.acc_y + s.acc_z * s.acc_z));
        float roll  = atan2f(s.acc_y, sqrtf(s.acc_x * s.acc_x + s.acc_z * s.acc_z));
        s_yaw += s.gyro_z * 0.001f;

        draw_cube(s_canvas, roll, pitch, s_yaw);
    } else {
        lv_label_set_text(s_label_acc_x, "--");
        lv_label_set_text(s_label_acc_y, "--");
        lv_label_set_text(s_label_acc_z, "--");
        lv_label_set_text(s_label_gyr_x, "--");
        lv_label_set_text(s_label_gyr_y, "--");
        lv_label_set_text(s_label_gyr_z, "--");
    }

    lv_label_set_text_fmt(s_label_steps, "%d", s.step_count);
}

static void page_imu_create(void)
{
    s_page_imu.screen = ui_create_screen_scrollable();
    lv_obj_t *scr = s_page_imu.screen;

    ui_create_page_title(scr, "IMU");

    int32_t y = ROUND_TOP_OFFSET + 28;

    s_canvas = lv_canvas_create(scr);
    lv_canvas_set_buffer(s_canvas, s_canvas_buf, CUBE_SIZE, CUBE_SIZE,
                         LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(s_canvas, 0, y);
    lv_obj_set_align(s_canvas, LV_ALIGN_TOP_MID);
    draw_cube(s_canvas, 0, 0, 0);

    y += CUBE_SIZE + 8;

    create_section_title(scr, y, "ACCEL (g)", lv_color_hex(0x4FC3F7));
    y += 20;
    s_label_acc_x = create_data_row(scr, y, "X:", lv_color_hex(0x4FC3F7));
    y += 24;
    s_label_acc_y = create_data_row(scr, y, "Y:", lv_color_hex(0x4FC3F7));
    y += 24;
    s_label_acc_z = create_data_row(scr, y, "Z:", lv_color_hex(0x4FC3F7));

    y += 32;
    create_section_title(scr, y, "GYRO (dps)", lv_color_hex(0x66BB6A));
    y += 20;
    s_label_gyr_x = create_data_row(scr, y, "X:", lv_color_hex(0x66BB6A));
    y += 24;
    s_label_gyr_y = create_data_row(scr, y, "Y:", lv_color_hex(0x66BB6A));
    y += 24;
    s_label_gyr_z = create_data_row(scr, y, "Z:", lv_color_hex(0x66BB6A));

    y += 32;
    create_section_title(scr, y, "STEPS", lv_color_hex(0xFFA726));
    y += 22;
    s_label_steps = lv_label_create(scr);
    lv_label_set_text(s_label_steps, "0");
    lv_obj_set_style_text_font(s_label_steps, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(s_label_steps, lv_color_hex(0xFFA726), 0);
    lv_obj_set_pos(s_label_steps, 0, y);
    lv_obj_set_align(s_label_steps, LV_ALIGN_TOP_MID);

    ui_add_gesture_back(scr);

    s_imu_timer = lv_timer_create(imu_timer_cb, 50, NULL);

    ESP_LOGI(TAG, "imu created");
}

static void page_imu_destroy(void)
{
    if (s_imu_timer) {
        lv_timer_delete(s_imu_timer);
        s_imu_timer = NULL;
    }
    s_canvas = NULL;
    s_label_acc_x = NULL;
    s_label_acc_y = NULL;
    s_label_acc_z = NULL;
    s_label_gyr_x = NULL;
    s_label_gyr_y = NULL;
    s_label_gyr_z = NULL;
    s_label_steps = NULL;
    s_page_imu.screen = NULL;
    ESP_LOGI(TAG, "imu destroyed");
}

static void page_imu_update(void)
{
}

ui_page_t *page_imu_get(void)
{
    if (!s_page_imu.create) {
        s_page_imu.create  = page_imu_create;
        s_page_imu.destroy = page_imu_destroy;
        s_page_imu.update  = page_imu_update;
    }
    return &s_page_imu;
}