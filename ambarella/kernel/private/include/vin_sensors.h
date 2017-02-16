#ifndef __VIN_SENSORS_H__
#define __VIN_SENSORS_H__

/* SENSOR IDs */
#define GENERIC_SENSOR			(127)	//changed to 127 in new DSP firmware

/* RGB */
#define SENSOR_RGB_1PIX			(0)
#define SENSOR_RGB_2PIX			(1)
/* YUV */
#define SENSOR_YUV_1PIX			(2)
#define SENSOR_YUV_2PIX			(3)

/* YUV pixel order */
#define SENSOR_CR_Y0_CB_Y1		(0)
#define SENSOR_CB_Y0_CR_Y1		(1)
#define SENSOR_Y0_CR_Y1_CB		(2)
#define SENSOR_Y0_CB_Y1_CR		(3)

/* Interface type */
#define SENSOR_PARALLEL_LVCMOS		(0)
#define SENSOR_SERIAL_LVDS		(1)
#define SENSOR_PARALLEL_LVDS		(2)
#define SENSOR_MIPI			(3)

/* Lane number */
#define SENSOR_1_LANE			(1)
#define SENSOR_2_LANE			(2)
#define SENSOR_3_LANE			(3)
#define SENSOR_4_LANE			(4)
#define SENSOR_6_LANE			(6)
#define SENSOR_8_LANE			(8)
#define SENSOR_10_LANE			(10)
#define SENSOR_12_LANE			(12)

/* Sync code style */
#define SENSOR_SYNC_STYLE_SONY		(0)
#define SENSOR_SYNC_STYLE_HISPI		(1)
#define SENSOR_SYNC_STYLE_ITU656		(2)
#define SENSOR_SYNC_STYLE_PANASONIC	(3)
#define SENSOR_SYNC_STYLE_SONY_DOL	(4)
#define SENSOR_SYNC_STYLE_HISPI_PSP	(5)
#define SENSOR_SYNC_STYLE_INTERLACE	(6)

/* vsync/hsync polarity */
#define SENSOR_VS_HIGH			(0x1 << 1)
#define SENSOR_VS_LOW			(0x0 << 1)
#define SENSOR_HS_HIGH			(0x1 << 0)
#define SENSOR_HS_LOW			(0x0 << 0)

/* data dege */
#define SENSOR_DATA_RISING_EDGE	(0)
#define SENSOR_DATA_FALLING_EDGE	(1)

/* sync mode */
#define SENSOR_SYNC_MODE_MASTER	(0)
#define SENSOR_SYNC_MODE_SLAVE	(1)

/* paralle_sync_type */
#define SENSOR_PARALLEL_SYNC_656	(0)
#define SENSOR_PARALLEL_SYNC_601	(1)

#endif

