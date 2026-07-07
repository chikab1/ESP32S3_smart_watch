#include "app_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

void ui_task(void *arg)
{
    qmi8658_data_t imu;
    cst816s_point_t tp;

    while (1)
    {
        memset(&imu, 0, sizeof(imu));
        memset(&tp, 0, sizeof(tp));

        xQueueReceive(imu_queue, &imu, 0);
        xQueueReceive(touch_queue, &tp, 0);

        if (tp.pressed) {
            printf("AX:%.2f AY:%.2f AZ:%.2f  TP:%d,%d\r\n",
                   imu.ax, imu.ay, imu.az,
                   tp.x, tp.y);
        } else {
            printf("AX:%.2f AY:%.2f AZ:%.2f  TP:---\r\n",
                   imu.ax, imu.ay, imu.az);
        }

        vTaskDelay(pdMS_TO_TICKS(33));
    }
}