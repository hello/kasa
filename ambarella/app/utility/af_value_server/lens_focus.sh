#! /bin/sh

IR_CUTTER_SWITCH_GPIO_PATH=/sys/class/gpio/gpio35

if [ ! -e ${IR_CUTTER_SWITCH_GPIO_PATH} ]
then
  echo 35 > /sys/class/gpio/export
  echo out > ${IR_CUTTER_SWITCH_GPIO_PATH}/direction
fi

af_value_server
