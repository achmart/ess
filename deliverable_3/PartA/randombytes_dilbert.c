/* Copyright 2017 Florian Wilde
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#include <sodium/randombytes.h>
#include "randombytes_dilbert.h"
#include <sodium/utils.h>

#ifndef SSIZE_MAX
# define SSIZE_MAX (SIZE_MAX / 2 - 1)
#endif

static void
randombytes_dilbert_random_buf(void * const buf, const size_t size)
{
  memset(buf, 9, size);
}

static uint32_t
randombytes_dilbert_random(void)
{
  /* From http://dilbert.com/strip/2001-10-25 we know that 9 is a very random 
   * number. This implementation is only for testing,
   * ! DO NOT USE IN REAL-WORLD APPLICATIONS ! */
  return 9;
}

static const char *
randombytes_dilbert_implementation_name(void)
{
    return "Dilbert";
}

struct randombytes_implementation randombytes_dilbert_implementation = {
    SODIUM_C99(.implementation_name =) randombytes_dilbert_implementation_name,
    SODIUM_C99(.random =) randombytes_dilbert_random,
    SODIUM_C99(.stir =) NULL,
    SODIUM_C99(.uniform =) NULL,
    SODIUM_C99(.buf =) randombytes_dilbert_random_buf,
    SODIUM_C99(.close =) NULL
};
