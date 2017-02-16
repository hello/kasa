/*******************************************************************************
 * test_am_log.cpp
 *
 * Histroy:
 *  2012-2-28 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "am_log.h"
#include "test_am_log.h"

void TestAmlogA::a()
{
  DEBUG("This is debug!");
  INFO("This is info!");
  NOTICE("This is NOTICE!");
  WARN("This is warning!");
  ERROR("This is error!");
}

void TestAmlogA::b()
{
  DEBUG("This is debug!");
  INFO("This is info!");
  NOTICE("This is notice!");
  WARN("This is warning!");
  ERROR("This is error!");
}

void TestAmlogA::c()
{
  DEBUG("This is debug!");
  INFO("This is info!");
  NOTICE("This is NOTICE!");
  WARN("This is warning!");
  ERROR("This is error!");
}

void TestAmlogA::d()
{
  DEBUG("This is debug!");
  INFO("This is info!");
  NOTICE("This is notice!");
  WARN("This is warning!");
  ERROR("This is error!");
}

void TestAmlogB::a()
{
  DEBUG("This is DEBUG!");
  INFO("This is info!");
  NOTICE("This is NOTICE!");
  WARN("This is warning!");
  ERROR("This is ERROR!");
}

void TestAmlogB::b()
{
  DEBUG("This is debug!");
  INFO("This is info!");
  NOTICE("This is notice!");
  WARN("This is warning!");
  ERROR("This is error!");
}

void TestAmlogB::c()
{
  DEBUG("This is debug!");
  INFO("This is info!");
  NOTICE("This is NOTICE!");
  WARN("This is warning!");
  ERROR("This is error!");
}

void TestAmlogB::d()
{
  DEBUG("This is debug!");
  INFO("This is info!");
  NOTICE("This is notice!");
  WARN("This is warning!");
  ERROR("This is ERROR!");
}
