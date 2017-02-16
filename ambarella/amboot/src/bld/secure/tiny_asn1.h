/**
 * tiny_asn1.h
 *
 * History:
 *  2015/06/25 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __TINY_ASN1_H__
#define __TINY_ASN1_H__

#ifdef __cplusplus
extern "C" {
#endif

#define ASN1_BOOLEAN                 0x01
#define ASN1_INTEGER                 0x02
#define ASN1_BIT_STRING              0x03
#define ASN1_OCTET_STRING            0x04
#define ASN1_NULL                    0x05
#define ASN1_OID                     0x06
#define ASN1_UTF8_STRING             0x0C
#define ASN1_SEQUENCE                0x10
#define ASN1_SET                     0x11
#define ASN1_PRINTABLE_STRING        0x13
#define ASN1_T61_STRING              0x14
#define ASN1_IA5_STRING              0x16
#define ASN1_UTC_TIME                0x17
#define ASN1_GENERALIZED_TIME        0x18
#define ASN1_UNIVERSAL_STRING        0x1C
#define ASN1_BMP_STRING              0x1E
#define ASN1_PRIMITIVE               0x00
#define ASN1_CONSTRUCTED             0x20
#define ASN1_CONTEXT_SPECIFIC        0x80

#define OID_SIZE(x) (sizeof(x) - 1)

typedef struct _asn1_buf {
    int tag;
    unsigned int len;
    unsigned char *p;
} asn1_buf;

int asn1_get_len(unsigned char** p, const unsigned char* end, unsigned int* len);
int asn1_get_tag(unsigned char** p, const unsigned char* end, unsigned int* len, int tag);
int asn1_write_len(unsigned char** p, unsigned char* start, unsigned int len);
int asn1_write_tag(unsigned char **p, unsigned char *start, unsigned char tag);

#ifdef __cplusplus
}
#endif

#endif

