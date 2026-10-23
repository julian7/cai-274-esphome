#pragma once

#include "esp_sntp.h"

inline bool force_sntp_sync() { return esp_sntp_restart(); }
