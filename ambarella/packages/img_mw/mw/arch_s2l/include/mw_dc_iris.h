/**********************************************************************
 * mw_dc_iris.h
 *
 * History:
 * 2014/09/17 - [Bin Wang] Created this file
 *
 * Copyright (C) 2014-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *********************************************************************/

#ifndef MW_DC_IRIS_H_
#define MW_DC_IRIS_H_

int enable_dc_iris(void);
int disable_dc_iris(void);
int dc_iris_set_pid_coef(mw_dc_iris_pid_coef * pPid_coef);
int dc_iris_get_pid_coef(mw_dc_iris_pid_coef * pPid_coef);
#endif /* MW_DC_IRIS_H_ */
