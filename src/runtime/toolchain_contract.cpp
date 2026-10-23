/** @file
 * @brief 在编译期核验值表示依赖的语言版本、字节宽度和 binary64 浮点条件。
 */
#include <climits>
#include <limits>
#include <span>

static_assert(__cplusplus == 202002L, "MYB requires C++20");
static_assert(CHAR_BIT == 8, "MYB requires 8-bit bytes");
static_assert(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559 &&
                  std::numeric_limits<double>::digits == 53 &&
                  std::numeric_limits<double>::max_exponent == 1024,
              "MYB requires IEEE-754 binary64 double");
static_assert(std::dynamic_extent == static_cast<std::size_t>(-1));
