/*******************************************************************************
 * am_plugin.h
 *
 * History:
 *   2014-7-18 - [ypchang] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
