#include "common/strop.h"

struct {
    encoding enc;
} g_strop_state = {ENCODING_UNKNOWN};

void strop_set_encoding(encoding enc)
{
    g_strop_state.enc = enc;
}
/*
uint64 strop_len_utf8(const uint8 *str)
{
    return
}

uint64 strop_len(const uint8 *str)
{

}

// compare two strings
// return an -1, 0, 1 if, respectively, str1 < str2, str1 == str2, str1 > str2
sint8 strop_cmp(const uint8 *str1, const uint8 *str2)
{

}
*/
