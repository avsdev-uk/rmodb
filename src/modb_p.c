#include "modb_p.h"
#include "strext.h"

char *modbTableName(struct modb_t *modb, const char *suffix, size_t suffix_len)
{
  str_builder *sb;
  char *str;
  size_t len;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }

  modbTableName_sb(sb, modb, suffix, suffix_len);

  if (strbld_finalize_or_destroy(&sb, &str, &len) != 0) {
    return 0;
  }

  return str;
}

void modbTableName_sb(str_builder *sb, struct modb_t *modb, const char *suffix, size_t suffix_len)
{
  strbld_str(sb, modb->name, modb->name_len);
  strbld_str(sb, suffix, suffix_len);
}
