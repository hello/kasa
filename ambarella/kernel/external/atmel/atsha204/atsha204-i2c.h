/* -*- mode: c; c-file-style: "linux" -*- */
/*
 * I2C Driver for Atmel ATSHA204 over I2C
 *
 * Copyright (C) 2014 Josh Datko, Cryptotronix, jbd@cryptotronix.com
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _ATSHA204_I2C_H_
#define _ATSHA204_I2C_H_

#include <linux/miscdevice.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/hw_random.h>
#include <linux/mutex.h>

#define ATSHA204_I2C_VERSION "0.1"
#define ATSHA204_SLEEP 0x01
#define ATSHA204_RNG_NAME "atsha-rng"

struct atsha204_chip {
    struct device *dev;

    int dev_num;
    char devname[7];
    unsigned long is_open;

    struct i2c_client *client;
    struct miscdevice miscdev;
    struct mutex transaction_mutex;
};

struct atsha204_cmd_metadata {
    int expected_rec_len;
    int actual_rec_len;
    unsigned long usleep;
};

struct atsha204_buffer {
    u8 *ptr;
    int len;
};

struct atsha204_file_priv {
    struct atsha204_chip *chip;
    struct atsha204_cmd_metadata meta;

    struct atsha204_buffer buf;
};

static const struct i2c_device_id atsha204_i2c_id[] = {
    {"atsha204-i2c", 0},
    { }
};


/* I2C detection */
int atsha204_i2c_probe(struct i2c_client *client,
                           const struct i2c_device_id *id);

int atsha204_i2c_remove(struct i2c_client *client);

/* Device registration */
struct atsha204_chip *atsha204_i2c_register_hardware(struct device *dev,
                                                     struct i2c_client *client);
int atsha204_i2c_add_device(struct atsha204_chip *chip);
void atsha204_i2c_del_device(struct atsha204_chip *chip);
int atsha204_i2c_release(struct inode *inode, struct file *filep);
int atsha204_i2c_open(struct inode *inode, struct file *filep);

/* atsha204 crc functions */
u16 atsha204_crc16(const u8 *buf, const u8 len);
bool atsha204_check_rsp_crc16(const u8 *buf, const u8 len);
int atsha204_i2c_validate_rsp(const struct atsha204_buffer *packet,
                                  struct atsha204_buffer *rsp);
void atsha204_i2c_crc_command(u8 *cmd, int len);

/* sysfs functions */
int atsha204_sysfs_add_device(struct atsha204_chip *chip);
void atsha204_sysfs_del_device(struct atsha204_chip *chip);

/* atsha204 specific functions */
int atsha204_i2c_wakeup(const struct i2c_client *client);
int atsha204_i2c_idle(const struct i2c_client *client);
int atsha204_i2c_transmit(const struct i2c_client *client,
                          const char __user *buf, size_t len);
int atsha204_i2c_transaction(struct atsha204_chip *chip,
                             const u8* to_send, size_t to_send_len,
                             struct atsha204_buffer *buf);
int atsha204_i2c_get_random(u8 *to_fill, const size_t max);

void atsha204_set_params(struct atsha204_cmd_metadata *cmd,
                         int expected_rec_len,
                         unsigned long usleep)
{
    cmd->expected_rec_len = expected_rec_len;
    cmd->usleep = usleep;
}

static int atsha204_i2c_rng_read(struct hwrng *rng, void *data,
                                 size_t max, bool wait)
{
    return atsha204_i2c_get_random(data, max);
}

static struct hwrng atsha204_i2c_rng = {
    .name = ATSHA204_RNG_NAME,
    .read = atsha204_i2c_rng_read,
};

/* Validation functions */
int validate_write_size(const size_t count);

#endif /* _ATSHA204_I2C_H_ */
