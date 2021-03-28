#include "FreeRTOS.h"
#include "event_groups.h"
#include "ff.h"
#include "gpio_lab.h"
#include "queue.h"
#include "ssp2_lab.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

QueueHandle_t producer_to_consumer;
EventGroupHandle_t checkin;
const uint32_t producer_bit = (1 << 1);
const uint32_t consumer_bit = (1 << 0);
const uint32_t all_bits = (consumer_bit | producer_bit);

void lights_off(void); // Turn off RED LEDs

void test_git(void);
// Sample code to write a file to the SD Card
// void write_file_using_fatfs_pi(void) {
//   const char *filename = "file.txt";
//   FIL file; // File handle
//   UINT bytes_written = 0;
//   FRESULT result = f_open(&file, filename, (FA_WRITE | FA_CREATE_ALWAYS));

//   if (FR_OK == result) {
//     char string[64];
//     sprintf(string, "Value,%i\n", 123);
//     if (FR_OK == f_write(&file, string, strlen(string), &bytes_written)) {
//     } else {
//       printf("ERROR: Failed to write data to file\n");
//     }
//     f_close(&file);
//   } else {
//     printf("ERROR: Failed to open: %s\n", filename);
//   }
// }

/**
  Create a producer task that reads a sensor value every 1ms.
    The sensor can be any input type, such as a light sensor, or an acceleration sensor
    After collecting 100 samples (after 100ms), compute the average
    Write average value every 100ms (avg. of 100 samples) to the sensor queue
    Use medium priority for this task
*/
void producer_task(void *p) {
  int *test_input = (int *)p;
  int average = 0;
  int index = 0;
  while (1) {
    index++;
    // test_ipnut = receive_data();
    average = *test_input;
    if (index == 100) {
      average = average / index;
      index = 0;
      if (!xQueueSend(producer_to_consumer, &average, 0)) {
        fprintf(stderr, "Error sending average\n");
      }
    }
    xEventGroupSetBits(checkin, producer_bit);
    vTaskDelay(100);
  }
  // while(1) { // Assume 100ms loop - vTaskDelay(100)
  //   // Sample code:
  //   // 1. get_sensor_value()
  //   // 2. xQueueSend(&handle, &sensor_value, 0);
  //   // 3. xEventGroupSetBits(checkin)
  //   // 4. vTaskDelay(100)
  // }
}

/**
  Create a consumer task that pulls the data off the sensor queue
    Use infinite timeout value during xQueueReceive API
    Open a file (i.e.: sensor.txt), and append the data to an output file on the SD card
    Save the data in this format: sprintf("%i, %i\n", time, light)"
    Note that you can get the time using xTaskGetTickCount()
    The sensor type is your choice (such as light or acceleration)
    Note that if you write and close a file every 100ms, it may be very inefficient, so try to come up with a better
  method such that the file is only written once a second or so... Also, note that periodically you may have to "flush"
  the file (or close it) otherwise data on the SD card may be cached and the file may not get written Use medium
  priority for this task
*/
void consumer_task(void *params) {
  int light = 0;
  int time = xTaskGetTickCount();
  while (1) {
    if (xQueueReceive(producer_to_consumer, &light, portMAX_DELAY)) {
      printf("Average: %i", light);
    }
    const char *filename = "sensor.txt";
    FIL file; // File handle
    UINT bytes_written = 0;
    FRESULT result = f_open(&file, filename, (FA_WRITE | FA_CREATE_ALWAYS));
    if (FR_OK == result) {
      char string[64];
      sprintf(string, "%i, %i\n", time, light);
      if (FR_OK == f_write(&file, string, strlen(string), &bytes_written)) {
      } else {
        printf("ERROR: Failed to write data to file\n");
      }
      f_close(&file);
    } else {
      printf("ERROR: Failed to open: %s\n", filename);
    }
  }
  // Assume 100ms loop
  // No need to use vTaskDelay() because the consumer will consume as fast as production rate
  // because we should block on xQueueReceive(&handle, &item, portMAX_DELAY);
  // Sample code:
  // 1. xQueueReceive(&handle, &sensor_value, portMAX_DELAY); // Wait forever for an item
  // 2. xEventGroupSetBits(checkin)
  xEventGroupSetBits(checkin, consumer_bit);
}

void watchdog_task(void *params) {
  while (1) {
    // ...
    // vTaskDelay(200);
    // We either should vTaskDelay, but for better robustness, we should
    // block on xEventGroupWaitBits() for slightly more than 100ms because
    // of the expected production rate of the producer() task and its check-in
    // if (xEventGroupWaitBits(...)) { // TODO
    //   // TODO
    // }
    uint32_t result = xEventGroupWaitBits(checkin, all_bits, pdTRUE, pdTRUE, 1000);
    if (result == all_bits) {
      fprintf(stderr, "all good\n");
    } else {
      if (!(result & producer_bit)) {
        fprintf(stderr, "producer not working\n");
      } else if (!(result & consumer_bit)) {
        fprintf(stderr, "consumer not working\n");
      }
    }
  }
}

int main(void) {
  lights_off();

  producer_to_consumer = xQueueCreate(1, sizeof(int));
  checkin = xEventGroupCreate();
  vTaskStartScheduler();
  return 0;
}

void lights_off(void) {
  gpio0__set_as_output(2, 3);
  gpio0__set_as_output(1, 26);
  gpio0__set_as_output(1, 24);
  gpio0__set_as_output(1, 18);
  gpio0__set_high(2, 3);
  gpio0__set_high(1, 26);
  gpio0__set_high(1, 24);
  gpio0__set_high(1, 18);
  printf("Lights off\n");
}