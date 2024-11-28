/*
            Example : Control Esp inbuilt led through Anedya command
*/

#ifndef _GETCOMMANDS_H_
#define _GETCOMMANDS_H_
#include "anedya_op_commands.h"

 typedef struct {
    int is_command_processed;
    anedya_command_obj_t * command_obj;
} command_handler_t;

extern command_handler_t command_handler;

void getCommands_task(void *pvParameters);

#endif // !_GETCOMMANDS_H_