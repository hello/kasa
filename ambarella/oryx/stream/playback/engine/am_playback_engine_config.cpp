/*******************************************************************************
 * am_playback_engine_config.cpp
 *
 * History:
 *   2014-8-20 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_playback_engine_config.h"
#include "am_configure.h"

AMPlaybackEngineConfig::AMPlaybackEngineConfig() :
  m_config(NULL),
  m_engine_config(NULL)
{}

AMPlaybackEngineConfig::~AMPlaybackEngineConfig()
{
  delete m_config;
  delete m_engine_config;
}

EngineConfig* AMPlaybackEngineConfig::get_config(const std::string& config_file)
{
  EngineConfig *config = NULL;
  if (AM_LIKELY(NULL == m_engine_config)) {
    m_engine_config = new EngineConfig();
  }
  delete m_config;
  m_config = AMConfig::create(config_file.c_str());
  if (AM_LIKELY(m_config)) {
    AMConfig& engine = *m_config;

    if (AM_LIKELY(engine["op_timeout"].exists() &&
                  engine["filters"].exists() &&
                  engine["connection"].exists())) {
      m_engine_config->op_timeout = engine["op_timeout"].get<unsigned>(5);
      m_engine_config->filter_num = engine["filters"].length();
      m_engine_config->filters = new std::string[m_engine_config->filter_num];
      m_engine_config->connections =
          new ConnectionConfig[m_engine_config->filter_num];
      if (AM_LIKELY(m_engine_config->filters &&
                    m_engine_config->connections)) {
        bool isOK = true;
        for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
          ConnectionConfig& connection = m_engine_config->connections[i];
          std::string *filter_name = &connection.filter;
          m_engine_config->filters[i] =
              engine["filters"][i].get<std::string>("");
          connection.filter = m_engine_config->filters[i];

          if (AM_LIKELY(engine["connection"][filter_name->c_str()].exists())) {
            /* Get Input Information */
            if (AM_LIKELY(engine["connection"][filter_name->c_str()]["input"].
                          exists())) {
              connection.input_number =
                  engine["connection"][filter_name->c_str()]["input"].length();
              if (AM_LIKELY(connection.input_number > 0)) {
                connection.input =
                    new ConnectionConfig*[connection.input_number];
                if (AM_UNLIKELY(!connection.input)) {
                  ERROR("Failed to allocate memory for input information!");
                  isOK = false;
                  break;
                }
              } else {
                INFO("Filter %s doesn't have input!",
                     connection.filter.c_str());
              }
            } else {
              INFO("Filter %s doesn't have input!", connection.filter.c_str());
            }
            /* Get Output Information */
            if (AM_LIKELY(engine["connection"][filter_name->c_str()]["output"].
                          exists())) {
              connection.output_number =
                  engine["connection"][filter_name->c_str()]["output"].length();
              if (AM_LIKELY(connection.output_number > 0)) {
                connection.output =
                    new ConnectionConfig*[connection.output_number];
                if (AM_UNLIKELY(!connection.output)) {
                  ERROR("Failed to allocate memory for output information!");
                  isOK = false;
                  break;
                }
              } else {
                INFO("Filter %s doesn't have output!",
                     connection.filter.c_str());
              }
            } else {
              INFO("Filter %s doesn't have output!", connection.filter.c_str());
            }
          } else {
            ERROR("Connection information is not defined for filter %s",
                  filter_name->c_str());
            isOK = false;
            break;
          }
        } /* for (uint32_t i = 0; i < m_engine_config->number; ++ i) */

        /* Build connections relations */
        if (AM_LIKELY(isOK)) {
          for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
            ConnectionConfig &connection = m_engine_config->connections[i];
            std::string filter = connection.filter;
            /* Find all input filter */
            for (uint32_t j = 0; j < connection.input_number; ++ j) {
              std::string input =
                  engine["connection"][filter.c_str()]["input"][j].
                  get<std::string>("");
              for (uint32_t x = 0; x < m_engine_config->filter_num; ++ x) {
                if (AM_LIKELY(m_engine_config->connections[x].filter ==
                    input)) {
                  connection.input[j] = &m_engine_config->connections[x];
                  break;
                }
              }
              if (AM_UNLIKELY(!connection.input[j])) {
                ERROR("Failed to find configuration of input filter %s",
                      input.c_str());
                isOK = false;
                break;
              }
            }
            if (AM_UNLIKELY(!isOK)) {
              break;
            }
            /* Find all output filter */
            for (uint32_t j = 0; j < connection.output_number; ++ j) {
              std::string output =
                  engine["connection"][filter.c_str()]["output"][j].
                  get<std::string>("");
              for (uint32_t x = 0; x < m_engine_config->filter_num; ++ x) {
                if (AM_LIKELY(m_engine_config->connections[x].filter ==
                    output)) {
                  connection.output[j] = &m_engine_config->connections[x];
                  break;
                }
              }
              if (AM_UNLIKELY(!connection.output[j])) {
                ERROR("Failed to find configuration of output filter %s",
                      output.c_str());
                isOK = false;
                break;
              }
            }
            if (AM_UNLIKELY(!isOK)) {
              break;
            }
          } /* for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) */
        }
        if (AM_LIKELY(isOK)) {
          config = m_engine_config;
        }
      } else {
        if (AM_LIKELY(!m_engine_config->filters)) {
          ERROR("Failed to allocate memory for filters information!");
        }
        if (AM_LIKELY(!m_engine_config->connections)) {
          ERROR("Failed to allocate memory for connection information!");
        }
      }
    } else {
      if (AM_LIKELY(!engine["op_timeout"].exists())) {
        ERROR("Invalid playback engine configuration file, "
              "\"op_timeout\" doesn't exist!");
      }
      if (AM_LIKELY(!engine["filters"].exists())) {
        ERROR("Invalid playback engine configuration file, "
              "\"filters\" doesn't exist!");
      }
      if (AM_LIKELY(!engine["connection"].exists())) {
        ERROR("Invalid playback engine configuration file, "
              "\"connection\" doesn't exist!");
      }
    }
  }

  return config;
}
