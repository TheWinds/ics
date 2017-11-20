#include "utils.h"

uint32_t str2uint32(char *str)
{
  uint32_t n = 0;
  int lenStr = strlen(str);
  for (int i = 0; i < lenStr; i++)
  {
    n += str[i] - '0';
    if (i != lenStr - 1)
      n *= 10;
  }
  return n;
}

uint32_t str2uint32_hex(char *str)
{
  uint32_t n = 0;
  int lenStr = strlen(str);
  for (int i = 0; i < lenStr; i++)
  {
    if (str[i] >= '0' && str[i] <= '9')
      n += str[i] - '0';
    if (str[i] >= 'A' && str[i] <= 'F')
      n += str[i] - 'A' + 10;
    if (str[i] >= 'a' && str[i] <= 'f')
      n += str[i] - 'a' + 10;
    if (i != lenStr - 1)
      n *= 16;
  }
  return n;
}