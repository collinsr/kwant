#ifndef INCL_SS_SYMBOL_H
#define INCL_SS_SYMBOL_H

namespace kwiver {
namespace kwant {

enum ss_symbol_types {INVALID, DOMAIN, ATTR, TIMEWINDOW, FILENAME}

class ss_symbol
{
public:
  static const size_t NULL_ID = 0;

  size_t my_id;
  size_t source_id;
  std::string name;
};

} // kwant
} // kwiver

#endif
