/*******************************************************************************
 * am_pm.h
 *
 * Histroy:
 *  2014-7-10         - [Louis ] created
 *
 * Copyright (C) 2008-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_PM_H_
#define AM_PM_H_

/*
Since S2L , PM has no relation to DPTZ, because PM is done before DPTZ, so no
need to do PM/DPTZ in same class, and no need to consider the synchronization
of PM and DPTZ , the design is a lot simpler.
 */

enum AM_PRIVACY_MASK_ACTION {
  AM_PM_ADD_INC = 0, /* add include region */
  AM_PM_ADD_EXC,     /* add exclude region */
  AM_PM_REPLACE,     /* replace with new region */
  AM_PM_REMOVE_ALL,  /* remove all regions */
  AM_PM_ACTIONS_NUM,
};

enum AM_PRIVACY_MASK_UNIT{
  AM_PM_UNIT_PERCENT = 0,
  AM_PM_UNIT_PIXEL,
};

typedef struct {
    int32_t id;
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
    uint32_t action;
} AMPRIVACY_MASK;

typedef struct {
    uint16_t start_x;
    uint16_t start_y;
    uint16_t width;
    uint16_t height;
    /* 0:Black, 1:Red, 2:Blue, 3:Green, 4:Yellow, 5:Magenta, 6:Cyan, 7:White */
    uint32_t color;
    uint32_t action;
} AMPRIVACY_MASK_RECT;

typedef struct _AMPrivacyMaskNode {
    int32_t group_index;
    AMPRIVACY_MASK pm_data;
    struct _AMPrivacyMaskNode * pNext;
} AMPrivacyMaskNode;

class AMPrivacyMask
{
  public:

    AMPrivacyMask(int iav_hd, VinParameters **vin);
    virtual ~AMPrivacyMask ();

    /**************** Privacy Mask related ****************/
    int32_t set_pm_param(PRIVACY_MASK * pm_in, RECT* buffer);
    int32_t get_pm_param(uint32_t * pm_in);
    /**************** Privacy Mask related ****************/

    int32_t disable_pm();
    int32_t pm_reset_all(RECT* buffer);

  protected:

    /**************** Privacy Mask related ****************/
    int32_t get_pm_rect(PRIVACY_MASK *pPrivacyMask, PRIVACY_MASK_RECT *rect);
    int32_t calc_pm_size(void);
    int32_t check_for_pm(PRIVACY_MASK_RECT *input_mask_pixel_rect);
    int32_t is_masked(int32_t block_x, int32_t block_y, PRIVACY_MASK_RECT
                      *block_rect);
    int32_t find_nearest_block_rect(PRIVACY_MASK_RECT *block_rect,
                                    PRIVACY_MASK_RECT *input_rect);
    int32_t add_privacy_mask(PRIVACY_MASK *pPrivacyMask);
    int32_t enable_privacy_mask(int32_t enable);
    int32_t get_transfer_info(DPTZParam *dptz);
    int32_t adjust_pm_in_main(PRIVACY_MASK *main_pm);
    int32_t trans_cord_vin_to_main(PRIVACY_MASK* main_pm, const PRIVACY_MASK
                                   *vin_pm);
    int32_t trans_cord_main_to_vin(PRIVACY_MASK* vin_pm,const PRIVACY_MASK
                                   *main_pm);
    int32_t pm_add_one_mask(PRIVACY_MASK* pm);
    int32_t pm_remove_all_mask();
    int32_t del_one_node(int32_t id);

  private:

    /**************** Privacy Mask related ****************/
    uint8_t pm_buffer_max_num;
    uint8_t pm_buffer_id;
    uint8_t pm_color_index;
    uint8_t g_pm_enable;
    uint16_t pm_buffer_pitch;
    uint32_t pm_buffer_size;
    uint32_t g_vin_width;
    uint32_t g_vin_height;
    uint32_t g_pm_color;
    int32_t num_block_x;
    int32_t num_block_y;

    PrivacyMaskNode* g_pm_head;
    PrivacyMaskNode* g_pm_tail;

    /**************** Privacy Mask related ****************/

    PRIVACY_MASK  g_pm;
    int m_iav;
};

#endif  /*AM_PM_H_ */
