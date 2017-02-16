/**********************************************************************
 * SensorModuleInfo.h
 *
 * Histroy:
 *  2011年03月17日 - [Yupeng Chang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 **********************************************************************/
#ifndef SENSOR_MODULE_INFO_H
#define SENSOR_MODULE_INFO_H

struct SensorModuleInfo {
    const char * module_name;
    unsigned int device_id;
    SensorModuleInfo () {module_name = 0; device_id = 0;}
    SensorModuleInfo (const char * name, unsigned int id)
    {
        module_name = name;
        device_id   = id;
    }
    void set_module_name (const char * name) {module_name = name;}
    const char * get_module_name () {return module_name;}
    void set_device_id (unsigned int id) {device_id = id;}
    unsigned int get_device_id () {return device_id;}
};

#endif //SENSOR_MODULE_INFO_H
