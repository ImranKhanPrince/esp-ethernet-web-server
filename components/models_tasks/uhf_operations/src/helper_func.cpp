#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "helper_func.h"
#include <cstdio>
#include <cstdlib>
#include <ctype.h>

char *addNewline(const char *str)
{
  char *newStr = (char *)malloc(strlen(str) + 2); // Allocate memory for the new string

  if (newStr != NULL)
  {
    strcpy(newStr, str);  // Copy the original string
    strcat(newStr, "\n"); // Add a newline at the end
  }

  return newStr;
}
void toLowerCase(char *str)
{
  while (*str)
  {
    *str = tolower((unsigned char)*str);
    str++;
  }
}
/*copies ssid and pass to wifi_config*/
char *u8cpy(uint8_t *dst, const uint8_t *src)
{
  return (char *)strcpy((char *)dst, (const char *)src);
}

unsigned char convertStringToUnsignedChar(const char *str)
{
  unsigned char result = 0;
  int scanned = std::sscanf(str, "%hhu", &result);

  // Check if the conversion was successful
  if (scanned != 1)
  {
    // Handle the error or provide a default value
    result = 0; // Default value in case of an error
  }

  return result;
}

const char *unsignedCharToString(unsigned char value)
{
  static char buffer[5]; // Adjust the size based on your requirement
  sprintf(buffer, "%u", value);
  return buffer;
}
const char *char_to_ms_string(unsigned char value)
{
  static char buffer[6]; // Adjust the size based on your requirement
  sprintf(buffer, "%lu", (long unsigned int)value * 100);
  return buffer;
}
const char *signedCharToString(signed char value)
{
  static char buffer[5]; // Adjust the size based on your requirement
  sprintf(buffer, "%d", value);
  return buffer;
}
const char *intToString(int value)
{
  static char buffer[12]; // Adjust the size based on your requirement
  sprintf(buffer, "%d", value);
  return buffer;
}
const char *unsignedShortToString(unsigned short value)
{
  static char buffer[12]; // Adjust the size based on your requirement
  sprintf(buffer, "%hu", value);
  return buffer;
}
const char *charArrayToString(char *array, int size)
{
  static char result[5]; // Adjust the size based on your requirement
  int index = 0;
  for (int i = 0; i < size; i++)
  {
    if (i == 1)
    {
      result[index++] = '.'; // Insert a decimal point after the first digit
    }
    index += snprintf(result + index, sizeof(result) - index, "%d", array[i]);
    // Serial.printf("%02X", array[i]);
  }
  return result;
}

uint32_t toLittleEndian(uint32_t value)
{
  return ((value & 0xFF000000) >> 24) | ((value & 0x00FF0000) >> 8) |
         ((value & 0x0000FF00) << 8) | ((value & 0x000000FF) << 24);
}
int charArrayToInt(const char array[])
{
  int result = 0;
  int i = 0;

  // Assuming the array represents a null-terminated string
  while (array[i] != '\0')
  {
    // Convert character to integer and update the result
    result = result * 10 + (array[i] - '0');
    i++;
  }

  return result;
}

void to_uppercase(char *str)
{
  while (*str)
  {
    *str = toupper(*str);
    str++;
  }
}

int hexstr_to_byte_array(const char *hexString, unsigned char *byteArray, size_t byteArraySize)
{
  size_t len = strlen(hexString);
  // Input string length is even cz each byte requires 2 string.
  // byteArray is at least half of the string
  if (len % 2 != 0 || byteArraySize < len / 2)
  {
    return -1;
  }
  for (size_t i = 0; i < len; i += 2)
  {
    unsigned int byte;
    if (sscanf(&hexString[i], "%2x", &byte) != 1)
    {
      return -1;
    }
    byteArray[i / 2] = (unsigned char)byte;
  }
  return 0; // Success
}
int byte_array_to_hexstr(const unsigned char *byteArray, size_t byteArraySize, char *hexString, size_t hexstr_size)
{
  if (hexstr_size < byteArraySize * 2 + 1)
  {
    return -1; // Insufficient buffer size
  }
  for (size_t i = 0; i < byteArraySize; i++)
  {
    sprintf(&hexString[i * 2], "%02X", byteArray[i]);
  }
  hexString[byteArraySize * 2] = '\0';
  return 0;
}

void print_byte_array(const unsigned char *byteArray, size_t length)
{
  printf("Byte array: ");
  for (size_t i = 0; i < length; ++i)
  {
    printf("0x%02X ", byteArray[i]);
  }
  printf("\n");
}