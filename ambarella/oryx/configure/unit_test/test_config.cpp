/*******************************************************************************
 * test_config.cpp
 *
 * History:
 *   2014-7-30 - [ypchang] created file
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

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"
#include "am_configure.h"
#include <string>
#include <valarray>

int main(int argc, char* argv[])
{
  std::string name;
  float mass = 0;
  float array[4];
  int i,j;
  int length;
  int m, n;
  float inertia_value;

  if (AM_UNLIKELY(argc != 2)) {
    ERROR("Usage: test_config config_name.lua");
  } else {
    AMConfig *config = AMConfig::create(argv[1]);
    if (AM_LIKELY(config)) {
      AMConfig& test = *config;

      PRINTF("Field 'name' %s !",
             test["name"].exists() ? "found" : "NOT found");
      name = test["name"].get<std::string>("");
      PRINTF("name found %s\n", name.c_str());

      if (test["nested"]["boolean"].exists()) {
        bool value = test["nested"]["boolean"].get<bool>(false);
        bool abc = false;

        PRINTF("Nested Boolean found!");
        PRINTF("Nested Boolean Value = %s", value ? "true" : "false");
        PRINTF("Set it to %s", value ? "false" : "true");
        test["nested"]["boolean"] = !value;
        abc = (bool)test["nested"]["boolean"];
        PRINTF("Nested Boolean Value is changed to %s", abc ? "true" : "false");
        if (AM_UNLIKELY(!test.save())) {
          ERROR("Failed to save!");
        }
      }

      if (test["group"].exists()) {
        m = test["group"].length();
        PRINTF("group = {");
        for (i = 0; i < m; ++ i) {
          std::string member = test["group"][i].get<std::string>("");
          PRINTF("    \"%s\",", member.c_str());
        }
        PRINTF("}");
        PRINTF("Add one group member: Member=%d", (i + 1));
        test["group"][i] = std::string("Member=") + std::to_string(i + 1);
        if (AM_UNLIKELY(!test.save())) {
          ERROR("Failed to save!");
        } else {
          m = test["group"].length();
          PRINTF("group = {");
          for (i = 0; i < m; ++ i) {
            std::string member = test["group"][i].get<std::string>("");
            PRINTF("    \"%s\",", member.c_str());
          }
          PRINTF("}");
        }
      }

      if (test["frames"]["thigh_right"]["child_body"]["mass"].exists()) {
        mass = test["frames"]["thigh_right"]["child_body"]["mass"].get<float>(0);
        PRINTF("mass is %f \n", mass);
      }

      //try to load array
      length = test["somearray"].length();
      PRINTF("somearray length is %d \n", length);
      PRINTF("somearray is: {", array[0]);
      for ( i = 0 ; i < length; i++) {
        array[i] = test["somearray"][i].get<float>(0);
        PRINTF("    %f, ", array[i]);
      }
      PRINTF("}\n");

      PRINTF("inertia length is %d \n", length);
      PRINTF("frames.pelvis.child_body.inertia = { \n");
      //try to load matrix
      m = test["frames"]["pelvis"]["child_body"]["inertia"].length();
      for (i = 0; i < m; i ++) {
        n = test["frames"]["pelvis"]["child_body"]["inertia"][i].length();
        for (j = 0; j < n; j ++) {
          inertia_value =
              test["frames"]["pelvis"]["child_body"]["inertia"][i][j].
              get<float>(0);
          PRINTF("    %f,", inertia_value);
        }
      }
      PRINTF("}\n");

      delete config;
    }
  }
}
