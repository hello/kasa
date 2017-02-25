/*******************************************************************************
 * runtime_config_xml.cpp
 *
 * History:
 *  2013/05/22 - [Zhi He] create file
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

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "media_mw_if.h"

#include "runtime_config_xml.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IRunTimeConfigAPI *gfCreateXMLRunTimeConfigAPI()
{
    IRunTimeConfigAPI *api = NULL;

    CXMLRunTimeConfigAPI *thiz = new CXMLRunTimeConfigAPI();
    if (thiz) {
        api = (IRunTimeConfigAPI *) thiz;
        return api;
    }

    LOG_FATAL("new CXMLRunTimeConfigAPI() fail\n");
    return NULL;
}

CXMLRunTimeConfigAPI::CXMLRunTimeConfigAPI():
    CObject("CXMLRunTimeConfigAPI"),
    mpDoc(NULL),
    mpRootNode(NULL),
    mpContext(NULL)
{
}

CXMLRunTimeConfigAPI::~CXMLRunTimeConfigAPI()
{
    if (mpContext) {
        xmlXPathFreeContext(mpContext);
        mpContext = NULL;
    }
    if (mpDoc) {
        xmlFreeDoc(mpDoc);
        mpDoc = NULL;
    }
}

EECode CXMLRunTimeConfigAPI::OpenConfigFile(const TChar *config_file_name, TU8 read_or_write, TU32 read_once)
{
    if (mpDoc) {
        LOG_ERROR("Config file already opened.\n");
        return EECode_Error;
    }
    mpDoc = xmlReadFile(config_file_name, "UTF-8", XML_PARSE_RECOVER);
    if (NULL == mpDoc) {
        LOG_FATAL("Document %s not parsed successfully.\n", config_file_name);
        return EECode_Error;
    }

    mpContext = xmlXPathNewContext(mpDoc);
    if (NULL == mpContext) {
        LOG_FATAL("Document %s context is NULL.\n", config_file_name);
        xmlFreeDoc(mpDoc);
        mpDoc = NULL;
        return EECode_Error;
    }

    //check config file root type
    xmlNodePtr root_node = xmlDocGetRootElement(mpDoc);
    if (NULL == root_node) {
        LOG_FATAL("Document %s is empty.\n", config_file_name);
        xmlXPathFreeContext(mpContext);
        mpContext = NULL;
        xmlFreeDoc(mpDoc);
        mpDoc = NULL;
        return EECode_Error;
    }

    return  EECode_OK;
}

EECode CXMLRunTimeConfigAPI::GetContentValue(const TChar *content_path, TChar *content_value)
{
    if (NULL == mpContext) {
        LOG_ERROR("Config file not opened, can not get content value.\n");
        return EECode_Error;
    }
    xmlXPathObjectPtr pValue = NULL;
    xmlNodeSetPtr pNodeset = NULL;
    xmlChar xpath[256];
    sprintf((char *)xpath, "//%s", (char *)content_path);
    pValue = xmlXPathEvalExpression(xpath, mpContext);
    if (NULL == pValue) {
        LOG_ERROR("GetContentValue, xmlXPathEvalExpression of path=%s return NULL\n", content_path);
        return EECode_Error;
    }
    pNodeset = pValue->nodesetval;
    if (xmlXPathNodeSetIsEmpty(pNodeset)) {
        xmlXPathFreeObject(pValue);
        LOG_ERROR("GetContentValue, nodeset of path=%s is empty\n", content_path);
        return EECode_Error;
    }
    //if (pNodeset->nodeNr > 1) {
    //    xmlXPathFreeObject(pValue);
    //    LOG_ERROR("GetContentValue, multi nodes match the path=%s, config file invalid.\n", content_path);
    //    return EECode_Error;
    //}
    strcpy((char *)content_value, (char *)xmlNodeGetContent(pNodeset->nodeTab[0]));
    LOG_INFO("GetContentValue, path=%s, value=%s\n", content_path, content_value);
    xmlXPathFreeObject(pValue);
    return EECode_OK;
}

EECode CXMLRunTimeConfigAPI::SetContentValue(const TChar *content_path, TChar *content_value)
{
    if (NULL == mpContext) {
        LOG_ERROR("Config file not opened, can not set content value.\n");
        return EECode_Error;
    }
    xmlXPathObjectPtr pValue = NULL;
    xmlNodeSetPtr pNodeset = NULL;
    xmlChar xpath[256];
    sprintf((char *)xpath, "//%s", (char *)content_path);
    pValue = xmlXPathEvalExpression(xpath, mpContext);
    if (NULL == pValue) {
        LOG_ERROR("SetContentValue, xmlXPathEvalExpression of path=%s return NULL\n", content_path);
        return EECode_Error;
    }
    pNodeset = pValue->nodesetval;
    if (xmlXPathNodeSetIsEmpty(pNodeset)) {
        xmlXPathFreeObject(pValue);
        LOG_ERROR("SetContentValue, nodeset of path=%s is empty\n", content_path);
        return EECode_Error;
    }
    if (pNodeset->nodeNr > 1) {
        xmlXPathFreeObject(pValue);
        LOG_ERROR("SetContentValue, multi nodes match the path=%s, config file invalid.\n", content_path);
        return EECode_Error;
    }
    xmlNodeSetContent(pNodeset->nodeTab[0], (const xmlChar *)content_value);
    LOG_INFO("SetContentValue, path=%s, value=%s\n", content_path, content_value);
    xmlXPathFreeObject(pValue);

    return EECode_OK;
}

EECode CXMLRunTimeConfigAPI::SaveConfigFile(const TChar *config_file_name)
{
    if (NULL == mpDoc) {
        LOG_ERROR("Config file not opened yet, can not save.\n");
        return EECode_Error;
    }
    if (-1 == xmlSaveFormatFileEnc(config_file_name, mpDoc, "UTF-8", 1)) {
        LOG_FATAL("SaveConfigFile to %s failed.\n", config_file_name);
        return EECode_Error;
    }

    return EECode_OK;
}

EECode CXMLRunTimeConfigAPI::CloseConfigFile()
{
    if (mpContext) {
        xmlXPathFreeContext(mpContext);
        mpContext = NULL;
    }
    if (mpDoc) {
        xmlFreeDoc(mpDoc);
        mpDoc = NULL;
    }

    return EECode_OK;
}

ERunTimeConfigType CXMLRunTimeConfigAPI::QueryConfigType() const
{
    return ERunTimeConfigType_XML;
}

EECode CXMLRunTimeConfigAPI::NewFile(const TChar *root_name, void *&root_node)
{
    DASSERT(root_name);
    DASSERT(!mpDoc);
    DASSERT(!mpRootNode);

    root_node = NULL;

    if (!root_name) {
        LOG_FATAL("NULL root_name %p\n", root_name);
        return EECode_BadParam;
    }

    if ((mpDoc) || (mpRootNode)) {
        LOG_FATAL("mpDoc %p or mpRootNode %p are not NULL?\n", mpDoc, mpRootNode);
        return EECode_BadState;
    }

    mpDoc = xmlNewDoc(BAD_CAST "1.0");
    mpRootNode = xmlNewNode(NULL, BAD_CAST root_name);
    DASSERT(mpDoc);
    DASSERT(mpRootNode);

    if ((!mpDoc) || (!mpRootNode)) {
        LOG_FATAL("NULL mpDoc %p or NULL mpRootNode %p\n", mpDoc, mpRootNode);
        return EECode_NoMemory;
    }

    xmlDocSetRootElement(mpDoc, mpRootNode);

    root_node = mpRootNode;
    return EECode_OK;
}

void *CXMLRunTimeConfigAPI::AddContent(void *p_parent, const TChar *content_name)
{
    xmlNodePtr parent = (xmlNodePtr)p_parent;
    xmlNodePtr new_node = NULL;

    DASSERT(mpDoc);
    DASSERT(mpRootNode);
    DASSERT(p_parent);
    DASSERT(content_name);

    if ((!p_parent) || (!content_name)) {
        LOG_FATAL("NULL p_parent %p, or NULL content_name %p\n", p_parent, content_name);
        return NULL;
    }

    if ((!mpDoc) || (!mpRootNode)) {
        LOG_FATAL("mpDoc %p or mpRootNode %p are NULL?\n", mpDoc, mpRootNode);
        return NULL;
    }

    new_node = xmlNewNode(NULL, BAD_CAST content_name);
    if (new_node) {
        xmlAddChild(parent, new_node);
    } else {
        LOG_FATAL("xmlNewNode(%s) fail\n", content_name);
        return NULL;
    }

    return (void *)new_node;
}

void *CXMLRunTimeConfigAPI::AddContentChild(void *p_parent, const TChar *child_name, const TChar *value)
{
    xmlNodePtr parent = (xmlNodePtr)p_parent;
    xmlNodePtr new_child = NULL;

    DASSERT(mpDoc);
    DASSERT(mpRootNode);
    DASSERT(p_parent);
    DASSERT(child_name);
    DASSERT(value);

    if ((!p_parent) || (!child_name) || (!value)) {
        LOG_FATAL("NULL p_parent %p, or NULL child_name %p, value %p\n", p_parent, child_name, value);
        return NULL;
    }

    if ((!mpDoc) || (!mpRootNode)) {
        LOG_FATAL("mpDoc %p or mpRootNode %p are NULL?\n", mpDoc, mpRootNode);
        return NULL;
    }

    new_child = xmlNewChild(parent, NULL, BAD_CAST child_name, BAD_CAST value);

    if (new_child) {
        xmlAddChild(parent, new_child);
    } else {
        LOG_FATAL("xmlNewChild(%s, %s) fail\n", child_name, value);
        return NULL;
    }

    return (void *)new_child;
}

EECode CXMLRunTimeConfigAPI::FinalizeFile(const TChar *file_name)
{
    DASSERT(mpDoc);
    DASSERT(mpRootNode);
    DASSERT(file_name);

    if ((!mpDoc) || (!mpRootNode)) {
        LOG_FATAL("mpDoc %p or mpRootNode %p are NULL?\n", mpDoc, mpRootNode);
        return EECode_BadState;
    }

    if (!file_name) {
        LOG_FATAL("NULL file_name\n");
        return EECode_BadParam;
    }

    if (-1 == xmlSaveFormatFileEnc(file_name, mpDoc, "UTF-8", 1)) {
        LOG_FATAL("FinalizeFile to %s failed.\n", file_name);
        return EECode_Error;
    }

    xmlFreeDoc(mpDoc);

    mpDoc = NULL;
    mpRootNode = NULL;

    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

