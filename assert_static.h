/* An assert alternative without memory allocations.
   This is important in this project beacsue the standard assert will call
   printf variants for printing, and those will call our malloc. If the assert
   happens from within a malloc, this will cause a deadlock.

   The assert here acts just like the standard assert:
     - If NDEBUG is defined, does nothing.
     - Otherwise, checks the condition, and if it fails, writes the filename and
       line to stderr, then aborts.
       Writing to stderr is best effort.

   Don't get confused between this assert and _Static_assert by gcc, which is a
   compile time assert.
*/

#ifndef __ASSERT_STATIC_H__
#define __ASSERT_STATIC_H__

#ifdef NDEBUG

#define assert(expr)

#else /* Not NDEBUG */

#include <unistd.h>

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

#define assert(expr)                                                    \
    do {                                                                \
        if (!(expr)) {                                                  \
            write(2, "assert failed: " __FILE__ ":" STRINGIZE(__LINE__) "\n", sizeof("assert failed: " __FILE__ ":" STRINGIZE(__LINE__) "\n") - 1); \
            abort();                                                    \
        }                                                               \
    } while (0);

#endif /* NDEBUG */

#endif /* __ASSERT_STATIC_H__ */
