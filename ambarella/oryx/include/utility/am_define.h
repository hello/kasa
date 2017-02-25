/*******************************************************************************
 * am_define.h
 *
 * Histroy:
 *  2012-3-1 2012 - [ypchang] created file
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

#ifndef AMDEFINE_H_
#define AMDEFINE_H_

#define ATTR_OFF       0
#define ATTR_BOLD      1
#define ATTR_BLINK     5
#define ATTR_REVERSE   7
#define ATTR_CONCEALED 8

#define FG_BLACK       30
#define FG_RED         31
#define FG_GREEN       32
#define FG_YELLOW      33
#define FG_BLUE        34
#define FG_MAGENTA     35
#define FG_CYAN        36
#define FG_WHITE       37

#define BG_BLACK       40
#define BG_RED         41
#define BG_GREEN       42
#define BG_YELLOW      43
#define BG_BLUE        44
#define BG_MAGENTA     45
#define BG_CYAN        46
#define BG_WHITE       47

#define B_RED(str)     "\033[1;31m" str "\033[0m"
#define B_GREEN(str)   "\033[1;32m" str "\033[0m"
#define B_YELLOW(str)  "\033[1;33m" str "\033[0m"
#define B_BLUE(str)    "\033[1;34m" str "\033[0m"
#define B_MAGENTA(str) "\033[1;35m" str "\033[0m"
#define B_CYAN(str)    "\033[1;36m" str "\033[0m"
#define B_WHITE(str)   "\033[1;37m" str "\033[0m"

#define RED(str)       "\033[31m" str "\033[0m"
#define GREEN(str)     "\033[32m" str "\033[0m"
#define YELLOW(str)    "\033[33m" str "\033[0m"
#define BLUE(str)      "\033[34m" str "\033[0m"
#define MAGENTA(str)   "\033[35m" str "\033[0m"
#define CYAN(str)      "\033[36m" str "\033[0m"
#define WHITE(str)     "\033[37m" str "\033[0m"

/*! @defgroup define Helper Macros and Functions
 * Helper macros and functions that are frequently used in programs.
 *  @{
 */ /* Start of document group */

/*! GCC prediction built-in function
 * @param x expected to be true
 */
#define AM_LIKELY(x)   (__builtin_expect(!!(x),1))

/*! GCC prediction built-in function
 * @param x expected to be false
 */
#define AM_UNLIKELY(x) (__builtin_expect(!!(x),0))

/*! AM_ABORT
 * @brief Abort macro defines.
 */
#ifdef AM_ORYX_DEBUG
#define AM_ABORT() abort()
#else
#define AM_ABORT() (void)0
#endif

/*! AM_ASSERT
 * @brief Assert macro defines.
 */
#define AM_ASSERT(expr) \
    do { \
      if (AM_UNLIKELY( !(expr) )) { \
        ERROR("assertion failed: %s", #expr); \
        AM_ABORT(); \
      } \
    } while (0);

/*! AM_DESTROY
 * @brief Destroy object macro defines.
 */
#define AM_DESTROY(_obj) \
    do { if (_obj) {(_obj)->destroy(); _obj = nullptr;}} while(0)

/*! AM_RELEASE
 * @brief Release object macro defines.
 */
#define AM_RELEASE(_obj) \
    do { if (_obj) {(_obj)->release(); _obj = nullptr;}} while(0)

/*! AM_ENSURE
 * @brief Ensure macro defines.
 */
#define AM_ENSURE(expr) \
    do { \
      if (AM_UNLIKELY(!(expr))) { \
        ERROR("test \"%s\" failed!", #expr); \
        AM_ABORT(); \
      } \
    } while(0)

/*! Compares the length of C style string a and b and return the less value.
 * @param a C style string, must NOT be NULL
 * @param b C style string, must NOT be NULL
 * @return string length of the less one
 */
#define minimum_str_len(a,b) \
  ((strlen(a) <= strlen(b)) ? strlen(a) : strlen(b))

/*! Test if a and b are equal, this is case insensitive
 * @param a C style string, must NOT be NULL
 * @param b C style string, must NOT be NULL
 * @return true if a and b are equal, otherwise return false
 */
#define is_str_equal(a,b) \
  ((strlen(a) == strlen(b)) && (0 == strcasecmp(a,b)))

/*! Test if the first n chars of a and b are equal, case insensitive.
 * @param a C style string, must NOT be NULL
 * @param b C style string, must NOT be NULL
 * @param n an integer greater than 0
 * @return true if the first n chars of a and b are equal,
 *         otherwise return false
 */
#define is_str_n_equal(a,b,n) \
  ((strlen(a) >= n) && (strlen(b) >= n) && (0 == strncasecmp(a,b,n)))

/*! Test if a and b are the same, this is case sensitive.
 * @param a C style string, must NOT be NULL
 * @param b C style string, must NOT be NULL
 * @return true if a and b are the same, otherwise return false
 */
#define is_str_same(a,b) \
  ((strlen(a) == strlen(b)) && (0 == strcmp(a,b)))

/*! Test if the first n chars of a and b are the same, case sensitive.
 * @param a C style string, must NOT be NULL
 * @param b C style string, must NOT be NULL
 * @param n an integer greater than 0
 * @return true if the first n chars of a and b are the same,
 *         otherwise return false
 */
#define is_str_n_same(a,b,n) \
  ((strlen(a) >= n) && (strlen(b) >= n) && (0 == strncmp(a,b,n)))

/*! Test if str contains prefix from the beginning.
 * @param str C style string, must NOT be NULL
 * @param prefix C style string, must NOT be NULL
 * @return true if str contains prefix from the beginning,
 *         otherwise false
 */
#define is_str_start_with(str,prefix) \
  ((strlen(str)>strlen(prefix))&&(0 == strncasecmp(str,prefix,strlen(prefix))))

/*! Roundup division.
 * ROUND_DIV(8, 3) = 3
 * @param dividend integer to be divided
 * @param divider  integer to divide dividend
 * @return the roundup value of equation dividend / divider
 */
#define ROUND_DIV(dividend, divider) (((dividend)+((divider)>>1)) / (divider))

/*! Roundup num to be multiplies of align
 * ROUND_UP(10, 4) = 12
 * @param num integer to be aligned up
 * @param align integer to be aligned to
 * @return the aligned value of num
 */
#define ROUND_UP(num, align) (((num) + ((align) - 1u)) & ~((align) - 1u))

/*! Rounddown num to be multiplies of align.
 * ROUND_DOWN(10, 4) = 8
 * @param num integer to be aligned down
 * @param align integer to be aligned to
 * @return the aligned value of num
 */
#define ROUND_DOWN(num, align) ((num) & ~((align) - 1))

/*! Return the absolute value.
 * @param x integer
 * @return the absolute value of x
 */
#define AM_ABS(x) (((x) < 0) ? -(x) : (x))

/*! Return the lesser value of A and B.
 * @param A integer
 * @param B integer
 * @return lesser value of A and B
 */
#define AM_MIN(A, B) ((A) < (B) ? (A) : (B))

/*! Return the greater value of A and B.
 * @param A integer
 * @param B integer
 * @return greater value of A and B
 */
#define AM_MAX(A, B) ((A) > (B) ? (A) : (B))

/*! Rethen the nth bit of integer x.
 * @param x integer
 * @param n integer indicates the bit position
 * @return the value of nth bit of integer x
 */
#define TEST_BIT(x, n) ((x) & (1 << (n)))

/*! Set the nth bit of integer x to 1.
 * @param x integer
 * @param n integer indicates the bit position
 */
#define SET_BIT(x, n) do {x = (x | ( 1 << (n)));} while(0);

/*! Set the nth bit of integer x to 0.
 * @param x integer
 * @param n integer indicates the bit position
 */
#define CLEAR_BIT(x, n) do { x = (x & (~( 1 << (n))));} while(0);

/*! Roundup a float type value to integer.
 * @param x float type number to be converted
 * @return integer
 */
#define FLOAT_TO_INT(x)  ((int)((x) + 0.5f))  // round off to int

/*! Roundup a double type value to signed long type
 * @param x double type number to be converted
 * @return signed long type
 */
#define DOUBLE_TO_LONG(x)  ((long)((x) + 0.5))  //round off to long

/*! Declare a variable is unused
 * @param x a variable
 */
#define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))

/*! Duplicate string str, return the newly created string.
 * if str is NULL, this function returns NULL, otherwise return the duplicated
 * C style string of str.
 * The memory of the return string is allocated by new command of C++, it should
 * be handled by this functions caller, and released by delete[] command of C++.
 * @param str const C style string
 * @return C style string
 */
char* amstrdup(const char* str);

/*! Get greatest common divider
 * @param a unsigned long long type
 * @param b unsigned long long type
 * @return the greatest common divider, which is also unsigned long long type
 */
unsigned long long get_gcd(unsigned long long a, unsigned long long b);

/*! Get least common multiple
 * @param a unsigned long long type
 * @param b unsigned long long type
 * @return the least common multiple of a and b,
 *         which is also unsigned long long type
 */
unsigned long long get_lcm(unsigned long long a, unsigned long long b);

/*! if data and data_len are valid, return the std::string containing the
 * encoded data, otherwise return an emtpy std::string
 * @param data const uint8_t* type, pointing to the data to be encoded
 * @param data_len indicates the length of the data pointed by data
 * @return std::string containing the encoded data or empty string.
 */
std::string base64_encode(const uint8_t *data, uint32_t data_len);

/*! Get 32bit random number
 */
uint32_t random_number();

/*!
 *@}
 */ /* End of Document Group */

#ifdef ENABLE_ORYX_PERFORMANCE_PROFILE
void print_current_time(const char *info);
#define PRINT_CURRENT_TIME(info) do { print_current_time(info); } while(0);
#else
#define PRINT_CURRENT_TIME(info)
#endif

#endif /* AMDEFINE_H_ */
