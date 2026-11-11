#pragma once


#define DEVBOARD 1 //if 1: testing sensor, has the VDD of the ir reader connected to GPIO_NUM_21

#define REPORTING 1 //if 1: ED reports cluster attributes, if 0: coordinator reads the attributes

extern volatile bool zb_busy;
void sml_init(void);
