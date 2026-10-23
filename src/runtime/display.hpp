#pragma once
#include "diagnostic/failure.hpp"
#include "runtime/value.hpp"
namespace myb {
class TextBuilder;
void display(Value, TextBuilder &, ErrorSite);
} // namespace myb
