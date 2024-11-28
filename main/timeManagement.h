#ifndef _TIMEMANAGEMENT_H_
#define _TIMEMANAGEMENT_H_

#include "freertos/FreeRTOS.h"
#include "sync.h"


void syncTime_task(void *pvParameters);
int64_t get_unix_current_time_ms(void);


#endif // !_TIMEMANAGEMENT_H_

