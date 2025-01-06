#pragma once
#include "settings.h"

#include <stdbool.h>
#include <vector>
#include <string>

#include "driver.h"

// cpp extern
std::vector<ScanResult> single_scan();

#ifdef __cplusplus
extern "C"
{
#endif

  bool stop_scan_task();

#ifdef __cplusplus
}
#endif