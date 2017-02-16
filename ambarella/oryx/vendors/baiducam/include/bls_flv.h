/*
The MIT License (MIT)

Copyright (c) 2013-2014 winlin

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef BLS_FLV_HPP
#define BLS_FLV_HPP

#ifdef __cplusplus
extern "C"{
#endif

/*
 * flv codec
 */
typedef void* bls_flv_t;
typedef int flv_bool;
/* open flv file for both read/write. */
bls_flv_t bls_flv_open_read(const char* file);
bls_flv_t bls_flv_open_write(const char* file);
void bls_flv_close(bls_flv_t flv);
/* read the flv header. 9bytes header. drop the 4bytes zero previous tag size */
int bls_flv_read_header(bls_flv_t flv, char header[9]);
/* read the flv tag header, 1bytes tag, 3bytes data_size, 4bytes time, 3bytes stream id. */
int bls_flv_read_tag_header(bls_flv_t flv, char* ptype, int32_t* pdata_size, u_int32_t* ptime);
/* read the tag data. drop the 4bytes previous tag size */
int bls_flv_read_tag_data(bls_flv_t flv, char* data, int32_t size);
/* write flv header to file, auto write the 4bytes zero previous tag size. */
int bls_flv_write_header(bls_flv_t flv, char header[9]);
/* write flv tag to file, auto write the 4bytes previous tag size */
int bls_flv_write_tag(bls_flv_t flv, char type, int32_t time, char* data, int size);
/* get the tag size, for flv injecter to adjust offset, size=tag_header+data+previous_tag */
int bls_flv_size_tag(int data_size);
/* file stream */
/* file stream tellg to get offset */
int64_t bls_flv_tellg(bls_flv_t flv);
/* seek file stream, offset is form the start of file */
void bls_flv_lseek(bls_flv_t flv, int64_t offset);
/* error code */
/* whether the error code indicates EOF */
flv_bool bls_flv_is_eof(int error_code);
/* media codec */
/* whether the video body is sequence header */
flv_bool bls_flv_is_sequence_header(char* data, int32_t size);
/* whether the video body is keyframe */
flv_bool bls_flv_is_keyframe(char* data, int32_t size);

/*
 * amf0 codec
 */
/* the output handler. */
typedef void* bls_amf0_t;
typedef int amf0_bool;
typedef double amf0_number;
bls_amf0_t bls_amf0_parse(char* data, int size, int* nparsed);
bls_amf0_t bls_amf0_create_number(amf0_number value);
bls_amf0_t bls_amf0_create_str(const char *value);
bls_amf0_t bls_amf0_create_ecma_array();
bls_amf0_t bls_amf0_create_strict_array();
bls_amf0_t bls_amf0_create_object();
void bls_amf0_free(bls_amf0_t amf0);
void bls_amf0_free_bytes(char* data);
/* size and to bytes */
int bls_amf0_size(bls_amf0_t amf0);
int bls_amf0_serialize(bls_amf0_t amf0, char* data, int size);
/* type detecter */
amf0_bool bls_amf0_is_string(bls_amf0_t amf0);
amf0_bool bls_amf0_is_boolean(bls_amf0_t amf0);
amf0_bool bls_amf0_is_number(bls_amf0_t amf0);
amf0_bool bls_amf0_is_null(bls_amf0_t amf0);
amf0_bool bls_amf0_is_object(bls_amf0_t amf0);
amf0_bool bls_amf0_is_ecma_array(bls_amf0_t amf0);
amf0_bool bls_amf0_is_strict_array(bls_amf0_t amf0);
/* value converter */
const char* bls_amf0_to_string(bls_amf0_t amf0);
amf0_bool bls_amf0_to_boolean(bls_amf0_t amf0);
amf0_number bls_amf0_to_number(bls_amf0_t amf0);
/* value setter */
void bls_amf0_set_number(bls_amf0_t amf0, amf0_number value);
/* object value converter */
int bls_amf0_object_property_count(bls_amf0_t amf0);
const char* bls_amf0_object_property_name_at(bls_amf0_t amf0, int index);
bls_amf0_t bls_amf0_object_property_value_at(bls_amf0_t amf0, int index);
bls_amf0_t bls_amf0_object_property(bls_amf0_t amf0, const char* name);
void bls_amf0_object_property_set(bls_amf0_t amf0, const char* name, bls_amf0_t value);
void bls_amf0_object_clear(bls_amf0_t amf0);
/* ecma array value converter */
int bls_amf0_ecma_array_property_count(bls_amf0_t amf0);
const char* bls_amf0_ecma_array_property_name_at(bls_amf0_t amf0, int index);
bls_amf0_t bls_amf0_ecma_array_property_value_at(bls_amf0_t amf0, int index);
bls_amf0_t bls_amf0_ecma_array_property(bls_amf0_t amf0, const char* name);
void bls_amf0_ecma_array_property_set(bls_amf0_t amf0, const char* name, bls_amf0_t value);
/* strict array value converter */
int bls_amf0_strict_array_property_count(bls_amf0_t amf0);
bls_amf0_t bls_amf0_strict_array_property_at(bls_amf0_t amf0, int index);
void bls_amf0_strict_array_append(bls_amf0_t amf0, bls_amf0_t value);
/*
 * human readable print
 * @param pdata, output the heap data, NULL to ignore.
 * user must use bls_amf0_free_bytes to free it.
 * @return return the *pdata for print. NULL to ignore.
 */
char* bls_amf0_human_print(bls_amf0_t amf0, char** pdata, int* psize);

#ifdef __cplusplus
}
#endif

#endif
