/*******************************************************************************
 * am_plugin.h
 *
 * History:
 *   2014-7-18 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_PLUGIN_H_
#define AM_PLUGIN_H_

/*! @file am_plugin.h
 *  @brief This file defines AMPlugin which is used to manage
 *         .so files which can be dynamically loaded and unloaded.
 */
#include <atomic>
#include <string>

/*! @addtogroup HelperClass
 *  @{
 */

/*! @class AMPlugin
 *  @brief Helper class defined to manage .so plugin files.
 *
 *  A .so file can be loaded into memory by dlopen and unloaded by dlclose,
 *  this class manages .so file, and provides simple APIs to access the symbol
 *  inside the .so file.
 *  A symbol inside .so file can be a function.
 */
class AMPlugin
{
  public:

    /*!
     * Constructor.
     * @param plugin C style string containing the path of a .so file.
     * @return AMPlugin pointer type, or NULL if failed to create.
     */
    static AMPlugin* create(const char *plugin);

    /*!
     * Get a symbol address inside the .so file managed by this class.
     * @param symbol C style string containing the symbol.
     * @return valid void* type address, or NULL if fail to get the symbol.
     */
    void* get_symbol(const char *symbol);

    /*!
     * Decrease the reference counter of this class. If the reference counter
     * reaches 0, destroy this class, and call dlclose to unload the .so file.
     */
    void destroy();

    /*!
     * Increase the reference counter of this class.
     */
    void add_ref();

  protected:
    bool init(const char *plugin);
    AMPlugin();
    virtual ~AMPlugin();

  protected:
    void           *m_handle;
    std::string     m_plugin;
    std::atomic_int m_ref;
};
/*!
 * @}
 */
#endif /* AM_PLUGIN_H_ */
