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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include <linux/uaccess.h>
#include <linux/crc16.h>
#include <linux/bitrev.h>
#include <linux/delay.h>
#include <linux/atomic.h>
#include <linux/printk.h>
#include "atsha204-i2c.h"

#define DNORMAL_LOG printk

#define DDEBUG_LOG printk
//#define DDEBUG_LOG dev_dbg

struct atsha204_chip *global_chip = NULL;
static atomic_t atsha204_avail = ATOMIC_INIT(1);

int atsha204_i2c_get_random(u8 *to_fill, const size_t max)
{
        int rc;
        struct atsha204_buffer recv = {0,0};
        int rnd_len;

        const u8 rand_cmd[] = {0x03, 0x07, 0x1b, 0x01, 0x00, 0x00, 0x27, 0x47};

        rc = atsha204_i2c_transaction(global_chip, rand_cmd, sizeof(rand_cmd),
                                      &recv);
        if (sizeof(rand_cmd) == rc){

                if (!atsha204_check_rsp_crc16(recv.ptr, recv.len)){
                        rc = -EBADMSG;
                        dev_err(global_chip->dev, "%s\n", "Bad CRC on Random");
                }
                else{
                        rnd_len = (max > recv.len - 3) ? recv.len - 3 : max;
                        memcpy(to_fill, &recv.ptr[1], rnd_len);
                        rc = rnd_len;
                        dev_info(global_chip->dev, "%s: %d\n",
                                 "Returning randoom bytes", rc);
                }

        }

        return rc;


}

int atsha204_i2c_transaction(struct atsha204_chip *chip,
                             const u8* to_send, size_t to_send_len,
                             struct atsha204_buffer *buf)


{
        int rc;
        u8 status_packet[4];
        u8 *recv_buf;
        int total_sleep = 60;
        int packet_len;

        mutex_lock(&chip->transaction_mutex);

        DDEBUG_LOG("%s\n", "About to send to device.");
        print_hex_dump_bytes("Sending : ", DUMP_PREFIX_OFFSET,
                             to_send, to_send_len);

        /* Begin i2c transactions */
        if ((rc = atsha204_i2c_wakeup(chip->client)))
                goto out;

        if ((rc = i2c_master_send(chip->client, to_send, to_send_len))
            != to_send_len)
                goto out;

        /* Poll for the response */
        while (4 != i2c_master_recv(chip->client, status_packet, 4)
               && total_sleep > 0){
                total_sleep = total_sleep - 4;
                msleep(4);
        }

        packet_len = status_packet[0];
        /* The device is awake and we don't want to hit the watchdog
           timer, so don't allow sleeps here*/
        recv_buf = kmalloc(packet_len, GFP_ATOMIC);
        memcpy(recv_buf, status_packet, sizeof(status_packet));
        rc = i2c_master_recv(chip->client, recv_buf + 4, packet_len - 4);

        atsha204_i2c_idle(chip->client);

        /* Store the entire packet. Other functions must check the CRC
           and strip of the length byte */
        buf->ptr = recv_buf;
        buf->len = packet_len;

        DDEBUG_LOG("%s\n", "Received from device.");
        print_hex_dump_bytes("Received: ", DUMP_PREFIX_OFFSET,
                             recv_buf, packet_len);

        rc = to_send_len;
out:
        mutex_unlock(&chip->transaction_mutex);
        return rc;

}


u16 atsha204_crc16(const u8 *buf, const u8 len)
{
        u8 i;
        u16 crc16 = 0;

        for (i = 0; i < len; i++) {
                u8 shift;

                for (shift = 0x01; shift > 0x00; shift <<= 1) {
                        u8 data_bit = (buf[i] & shift) ? 1 : 0;
                        u8 crc_bit = crc16 >> 15;

                        crc16 <<= 1;

                        if ((data_bit ^ crc_bit) != 0)
                                crc16 ^= 0x8005;
                }
        }

        return cpu_to_le16(crc16);
}

bool atsha204_crc16_matches(const u8 *buf, const u8 len, const u16 crc)
{
        u16 crc_calc = atsha204_crc16(buf,len);
        return (crc == crc_calc) ? true : false;
}

bool atsha204_check_rsp_crc16(const u8 *buf, const u8 len)
{
        u16* rec_crc = (u16*) &buf[len - 2];
        return atsha204_crc16_matches(buf, len - 2, cpu_to_le16(*rec_crc));
}

int atsha204_i2c_validate_rsp(const struct atsha204_buffer *packet,
                              struct atsha204_buffer *rsp)
{
        int rc;

        if (packet->len < 4)
                goto out_bad_msg;
        else if (atsha204_check_rsp_crc16(packet->ptr, packet->len)){
                rsp->ptr = packet->ptr + 1;
                rsp->len = packet->len - 3;
                rc = 0;
                goto out;
        }
        else
                /* CRC failed */

out_bad_msg:
        rc = -EBADMSG;
out:
        return rc;
}

int i2c_master_send_without_ack(const struct i2c_client *client, const char *buf, int count)
{
	int ret;
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;

	msg.addr = client->addr;
	msg.flags = I2C_M_IGNORE_NAK;
	msg.len = count;
	msg.buf = (char *)buf;

	ret = i2c_transfer(adap, &msg, 1);
	return (ret == 1) ? count : ret;
}

int read_wakeup_status(const struct i2c_client *client)
{
	u8 buf[16];
	int ret = 0;

	buf[0] = 0x04;
	buf[1] = 0x11,
	buf[2] = 0x33,
	buf[3] = 0x43;
	ret = i2c_master_recv(client, buf, 4);
        if (4 == ret) {
		DNORMAL_LOG("[atsha204]: read_wakeup_status() 1\n");
	} else {
		printk("step1 fail\n");
	}

	buf[0] = 0x03;
	buf[1] = 0x0b;
	buf[2] = 0x12;
	buf[3] = 0x00;
	buf[4] = 0x00;
	buf[5] = 0x00;
	buf[6] = 0x00;
	buf[7] = 0x00;
	buf[8] = 0x00;
	buf[9] = 0x00;
	buf[10] = 0xA7;
	buf[11] = 0xBF;

	ret = i2c_master_send(client, buf, 12);
        if (12 == ret) {
		DNORMAL_LOG("[atsha204]: read_wakeup_status() 2\n");
	} else {
		printk("step2 fail\n");
	}

	buf[0] = 0x04;
	buf[1] = 0x03,
	buf[2] = 0x83,
	buf[3] = 0x42;
	ret = i2c_master_recv(client, buf, 4);
        if (4 == ret) {
		DNORMAL_LOG("[atsha204]: read_wakeup_status() 3\n");
	} else {
		printk("step3 fail\n");
	}

#if 0
	buf[0] = 0x02;

	ret = i2c_master_send(client, buf, 1);
        if (1 == ret) {
		DNORMAL_LOG("[atsha204]: read_wakeup_status() 4\n");
	} else {
		printk("step4 fail\n");
	}
#endif

	return 0;
}

int atsha204_i2c_wakeup(const struct i2c_client *client)
{
        bool is_awake = false;
        int retval = -ENODEV;

        u8 buf[4] = {0};
	int ret = 0;

        unsigned short int try_con = 1;

	DNORMAL_LOG("[atsha204]: atsha204_i2c_wakeup()\n");

        while (!is_awake) {
		ret = i2c_master_send_without_ack(client, buf, 4);
                if (4 == ret){
                        pr_info("%s\n", "ATSHA204 Device is awake.");
                        is_awake = true;

                        if (4 == i2c_master_recv(client, buf, 4)) {
                                pr_info("%s", "ATSHA204 Received wakeup\n");
                        }

                        if (atsha204_check_rsp_crc16(buf,4))
                                retval = 0;
                        else
                                pr_err("%s\n", "ATSHA204 Wakeup CRC failure");


                }
                else{
                        /* is_awake is already false */
                        pr_info("Attempting Wakeup : %u, ret %d\n", try_con, ret);
                        if(try_con >= 10){
                                pr_err("Wakeup Failed. No Device");
                                return retval;
                        }
                }

                ++try_con;
        }

	//read_wakeup_status(client);

        retval = 0;
        return retval;

}

int atsha204_i2c_idle(const struct i2c_client *client)
{
        int rc;

        u8 idle_cmd[1] = {0x02};

	DNORMAL_LOG("[atsha204]: atsha204_i2c_idle()\n");

        rc = i2c_master_send(client, idle_cmd, 1);

        return rc;

}

int atsha204_i2c_sleep(const struct i2c_client *client)
{
        int retval;
        char to_send[1] = {ATSHA204_SLEEP};

	DNORMAL_LOG("[atsha204]: atsha204_i2c_sleep()\n");

        if ((retval = i2c_master_send(client,to_send,1)) == 1)
                retval = 0;
        else
                pr_err("%s: 0x%x\n", "ATSHA204 failed to sleep", client->addr);

        return retval;

}

void atsha204_i2c_crc_command(u8 *cmd, int len)
{
        /* The command packet is:
           [0x03] [Length=1 + command + CRC] [cmd] [crc]

           The CRC is calculated over:
           CRC([Len] [cmd]) */
        int crc_data_len = len - 2 - 1;
        u16 crc = atsha204_crc16(&cmd[1], crc_data_len);


        cmd[len - 2] = cpu_to_le16(crc) & 0xFF;
        cmd[len - 1] = cpu_to_le16(crc) >> 8;
}

int validate_write_size(const size_t count)
{
        const int MIN_SIZE = 4;
        /* Header and CRC occupy 4 bytes and the length is a one byte
           value */
        const int MAX_SIZE = 255 - 4;
        int rc = -EMSGSIZE;

        if (count <= MAX_SIZE && count >= MIN_SIZE)
                rc = 0;

        return rc;

}
ssize_t atsha204_i2c_write(struct file *filep, const char __user *buf,
                           size_t count, loff_t *f_pos)
{
        struct atsha204_file_priv *priv = filep->private_data;
        struct atsha204_chip *chip = priv->chip;
        u8 *to_send;
        int rc;

        /* Add command byte + length + 2 byte crc */
        const int SEND_SIZE = count + 4;
        const u8 COMMAND_BYTE = 0x03;

	DDEBUG_LOG("[atsha204]: atsha204_i2c_write()\n");

        if ((rc = validate_write_size(count)))
                return rc;

        to_send = kmalloc(SEND_SIZE, GFP_KERNEL);
        if (!to_send)
                return -ENOMEM;

        /* Write the header */
        to_send[0] = COMMAND_BYTE;
        /* Length byte = user size + crc size + length byte */
        to_send[1] = count + 2 + 1;

        if (copy_from_user(&to_send[2], buf, count)){
                rc = -EFAULT;
                kfree(to_send);
                return rc;
        }

        atsha204_i2c_crc_command(to_send, SEND_SIZE);

        rc = atsha204_i2c_transaction(chip, to_send, SEND_SIZE,
                                      &priv->buf);

        /* Return to the user the number of bytes that the
           user provided, don't include the extra header / crc
           bytes */
        if (SEND_SIZE == rc)
                rc = count;

        /* Reset the f_pos, which indicates the read position in the
           buffer. Byte 1 points at the start of the data */
        *f_pos = 1;

        kfree(to_send);

        return rc;
}

ssize_t atsha204_i2c_read(struct file *filep, char __user *buf, size_t count,
                          loff_t *f_pos)
{
        struct atsha204_file_priv *priv = filep->private_data;
        struct atsha204_chip *chip = priv->chip;
        struct atsha204_buffer *r_buf = &priv->buf;
        ssize_t rc = 0;
        /* r_buf has 3 extra bytes that should not be returned to the
           user. The first byte (length) and the last two (crc).
           However, since f_pos is reset to 1 on write, only subtract
           2 here.
        */
        const int MAX_REC_LEN = r_buf->len - 2;

	DDEBUG_LOG("[atsha204]: atsha204_i2c_read()\n");

        /* Check the CRC on the rec buffer on the first read */
        if (*f_pos == 1 && !atsha204_check_rsp_crc16(r_buf->ptr, r_buf->len)){
                rc = -EBADMSG;
                dev_err(chip->dev, "%s\n", "CRC on received buffer failed.");
                goto out;
        }

        if (*f_pos >= MAX_REC_LEN)
                goto out;

        if (*f_pos + count > MAX_REC_LEN)
                count = MAX_REC_LEN - *f_pos;

        if ((rc = copy_to_user(buf, &r_buf->ptr[*f_pos], count))){
                rc = -EFAULT;
        }
        else{
                *f_pos += count;
                rc = count;
        }

out:
        return rc;
}


int atsha204_i2c_open(struct inode *inode, struct file *filep)
{
        struct miscdevice *misc = filep->private_data;
        struct atsha204_chip *chip = container_of(misc, struct atsha204_chip,
                                                  miscdev);
        struct atsha204_file_priv *priv;

	DNORMAL_LOG("[atsha204]: atsha204_i2c_open()\n");

        if (!atomic_dec_and_test(&atsha204_avail)){
                atomic_inc(&atsha204_avail);
                return -EBUSY;
        }

        priv = kzalloc(sizeof(*priv), GFP_KERNEL);
        if (NULL == priv)
                return -ENOMEM;

        priv->chip = chip;

        filep->private_data = priv;

        filep->f_pos = 0;

        return 0;

}


int atsha204_i2c_release(struct inode *inode, struct file *filep)
{
	DNORMAL_LOG("[atsha204]: atsha204_i2c_release()\n");

        atomic_inc(&atsha204_avail);

        return 0;
}




struct atsha204_chip *atsha204_i2c_register_hardware(struct device *dev,
                                                     struct i2c_client *client)
{

        struct atsha204_chip *chip;

        if ((chip = kzalloc(sizeof(*chip), GFP_KERNEL)) == NULL)
                goto out_null;

        chip->dev_num = 0;
        scnprintf(chip->devname, sizeof(chip->devname), "%s%d",
                  "atsha", chip->dev_num);

        chip->dev = get_device(dev);
        dev_set_drvdata(dev, chip);

        chip->client = client;

        mutex_init(&chip->transaction_mutex);

        if (atsha204_i2c_add_device(chip)){
                dev_err(dev, "%s\n", "Failed to add device");
                goto put_device;
        }
        else{
                int rc = hwrng_register(&atsha204_i2c_rng);
                DDEBUG_LOG("%s%d\n", "HWRNG result: ", rc);
        }


        return chip;

put_device:
        put_device(chip->dev);

        kfree(chip);
out_null:
        return NULL;
}


int atsha204_i2c_probe(struct i2c_client *client,
                       const struct i2c_device_id *id)
{
        int result = -1;
        struct device *dev = &client->dev;
        struct atsha204_chip *chip;

	DNORMAL_LOG("[atsha204]: atsha204_i2c_probe()\n");

        if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
                return -ENODEV;

        if((result = atsha204_i2c_wakeup(client)) == 0){
                pr_debug("%s: 0x%x\n", "ATSHA204 Device is awake",
                         client->addr);

                atsha204_i2c_idle(client);

                if ((chip = atsha204_i2c_register_hardware(dev, client))
                    == NULL){
                        return -ENODEV;
                }

                else {
                        global_chip = chip;
                }

                result = atsha204_sysfs_add_device(chip);
        }

        else{
                pr_err("%s: 0x%x\n", "ATSHA204 device failed to wake",
                       client->addr);
        }

        return result;
}

int atsha204_i2c_remove(struct i2c_client *client)

{
        struct device *dev = &(client->dev);
        struct atsha204_chip *chip = dev_get_drvdata(dev);

	DNORMAL_LOG("[atsha204]: atsha204_i2c_remove()\n");

        if (chip){
                misc_deregister(&chip->miscdev);
                atsha204_sysfs_del_device(chip);
        }

        hwrng_unregister(&atsha204_i2c_rng);

        kfree(chip);

        global_chip = NULL;

        /* The device is in an idle state, where it keeps ephemeral
         * memory. Wakeup the device and sleep it, which will cause it
         * to clear its internal memory */

        atsha204_i2c_wakeup(client);
        atsha204_i2c_sleep(client);

        return 0;

}





MODULE_DEVICE_TABLE(i2c, atsha204_i2c_id);

static struct i2c_driver atsha204_i2c_driver = {
        .driver = {
                .name = "atsha204-i2c",
                .owner = THIS_MODULE,
        },
        .probe = atsha204_i2c_probe,
        .remove = atsha204_i2c_remove,
        .id_table = atsha204_i2c_id,
};

static int __init atsha204_i2c_init(void)
{
	DDEBUG_LOG("[atsha204]: atsha204_i2c_init()\n");
        return i2c_add_driver(&atsha204_i2c_driver);
}


static void __exit atsha204_i2c_driver_cleanup(void)
{
        i2c_del_driver(&atsha204_i2c_driver);
}
module_init(atsha204_i2c_init);
module_exit(atsha204_i2c_driver_cleanup);

static const struct file_operations atsha204_i2c_fops = {
        .owner = THIS_MODULE,
        .llseek = no_llseek,
        .open = atsha204_i2c_open,
        .read = atsha204_i2c_read,
        .write = atsha204_i2c_write,
        .release = atsha204_i2c_release,
};


int atsha204_i2c_add_device(struct atsha204_chip *chip)
{
        int retval;

        chip->miscdev.fops = &atsha204_i2c_fops;
        chip->miscdev.minor = MISC_DYNAMIC_MINOR;

        chip->miscdev.name = chip->devname;
        chip->miscdev.parent = chip->dev;

        if ((retval = misc_register(&chip->miscdev)) != 0){
                chip->miscdev.name = NULL;
                dev_err(chip->dev,
                        "unable to misc_register %s, minor %d err=%d\n",
                        chip->miscdev.name,
                        chip->miscdev.minor,
                        retval);
        }


        return retval;
}

int atsha204_i2c_read4(struct atsha204_chip *chip, u8 *read_buf,
                       const u16 addr, const u8 param1)
{
        u8 read_cmd[8] = {0};
        u16 crc;
        struct atsha204_buffer rsp, msg;
        int rc, validate_status;

        read_cmd[0] = 0x03; /* Command byte */
        read_cmd[1] = 0x07; /* length */
        read_cmd[2] = 0x02; /* Read command opcode */
        read_cmd[3] = param1;
        read_cmd[4] = cpu_to_le16(addr) & 0xFF;
        read_cmd[5] = cpu_to_le16(addr) >> 8;

        crc = atsha204_crc16(&read_cmd[1], 5);


        read_cmd[6] = cpu_to_le16(crc) & 0xFF;
        read_cmd[7] = cpu_to_le16(crc) >> 8;

        rc = atsha204_i2c_transaction(chip, read_cmd,
                                      sizeof(read_cmd), &rsp);

        if (sizeof(read_cmd) == rc){
                if ((validate_status = atsha204_i2c_validate_rsp(&rsp, &msg))
                    == 0){
                        memcpy(read_buf, msg.ptr, msg.len);
                        kfree(rsp.ptr);
                        rc = msg.len;
                }
                else
                        rc = validate_status;

        }

        return rc;



}


static ssize_t configzone_show(struct device *dev,
                               struct device_attribute *attr,
                               char *buf)
{
        struct atsha204_chip *chip = dev_get_drvdata(dev);
        int i;
        u16 bytes, word_addr;
        bool keep_going = true;
        u8 param1 = 0; /* indicates configzone region */
        char *str = buf;

        u8 configzone[128] = {0};

        for (bytes = 0; bytes < sizeof(configzone) && keep_going; bytes += 4){
                word_addr = bytes / 4;
                if (4 != atsha204_i2c_read4(chip,
                                            &configzone[bytes],
                                            word_addr, param1)){
                        keep_going = false;
                }
        }

        for (i = 0; i < bytes; i++) {
                str += sprintf(str, "%02X ", configzone[i]);
                if ((i + 1) % 4 == 0)
                        str += sprintf(str, "\n");
        }

        return str - buf;
}
/*static DEVICE_ATTR_RO(configzone);*/
struct device_attribute dev_attr_configzone = __ATTR_RO(configzone);

static ssize_t serialnum_show(struct device *dev,
                              struct device_attribute *attr,
                              char *buf)
{
        struct atsha204_chip *chip = dev_get_drvdata(dev);
        int i;
        u16 bytes, word_addr;
        bool keep_going = true;
        u8 param1 = 0; /* indicates configzone region */
        char *str = buf;

        u8 serial[12] = {0};

        for (bytes = 0; bytes < sizeof(serial) && keep_going; bytes += 4){
                word_addr = bytes / 4;
                if (4 != atsha204_i2c_read4(chip,
                                            &serial[bytes],
                                            word_addr, param1)){
                        keep_going = false;
                }
        }

        for (i = 0; i < bytes; i++) {
                str += sprintf(str, "%02X", serial[i]);
                if ((i + 1) % sizeof(serial) == 0)
                        str += sprintf(str, "\n");
        }

        return str - buf;
}
/*static DEVICE_ATTR_RO(configzone);*/
struct device_attribute dev_attr_serialnum = __ATTR_RO(serialnum);

static ssize_t is_locked(struct device *dev,
                         struct device_attribute *attr,
                         char *buf,
                         const int offset)
{
        struct atsha204_chip *chip = dev_get_drvdata(dev);
        const u16 LOCK_ADDR = 0x15;
        u8 param1 = 0; /* indicates configzone region */
        char *str = buf;
        u8 lock_buf[4];
        const u8 UNLOCKED = 0x55;

        if (sizeof(lock_buf) == atsha204_i2c_read4(chip,
                                                   lock_buf,
                                                   LOCK_ADDR, param1)){

                if (UNLOCKED == lock_buf[offset])
                        str += sprintf(str, "0");
                else
                        str += sprintf(str, "1");

                str += sprintf(str, "\n");

        }

        return str - buf;
}

static ssize_t configlocked_show(struct device *dev,
                                 struct device_attribute *attr,
                                 char *buf)
{
        const int CONFIG_ZONE_LOCK_OFFSET = 3;

        return is_locked(dev, attr, buf, CONFIG_ZONE_LOCK_OFFSET);
}
struct device_attribute dev_attr_configlocked = __ATTR_RO(configlocked);

static ssize_t datalocked_show(struct device *dev,
                               struct device_attribute *attr,
                               char *buf)
{
        const int DATA_ZONE_LOCK_OFFSET = 2;

        return is_locked(dev, attr, buf, DATA_ZONE_LOCK_OFFSET);
}
struct device_attribute dev_attr_datalocked = __ATTR_RO(datalocked);

static struct attribute *atsha204_dev_attrs[] = {
        &dev_attr_configzone.attr,
        &dev_attr_serialnum.attr,
        &dev_attr_configlocked.attr,
        &dev_attr_datalocked.attr,
        NULL,
};

static const struct attribute_group atsha204_dev_group = {
        .attrs = atsha204_dev_attrs,
};

int atsha204_sysfs_add_device(struct atsha204_chip *chip)
{
        int err;
        err = sysfs_create_group(&chip->dev->kobj,
                                 &atsha204_dev_group);

        if (err)
                dev_err(chip->dev,
                        "failed to create sysfs attributes, %d\n", err);
        return err;
}

void atsha204_sysfs_del_device(struct atsha204_chip *chip)
{
        sysfs_remove_group(&chip->dev->kobj, &atsha204_dev_group);
}

MODULE_AUTHOR("Josh Datko <jbd@cryptotronix.com");
MODULE_DESCRIPTION("Atmel ATSHA204 driver");
MODULE_VERSION(ATSHA204_I2C_VERSION);
MODULE_LICENSE("GPL");
