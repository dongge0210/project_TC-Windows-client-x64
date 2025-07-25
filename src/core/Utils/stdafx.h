#pragma once

// Common C++ Standard Library includes
#include <string>
#include <fstream>
#include <mutex>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

// Windows specific includes
#include <windows.h>
#include <io.h>
#include <fcntl.h>

// C++20 specific headers if needed
#ifdef __cplusplus
#if __cplusplus >= 202002L
#include <concepts>
#include <span>
#endif
#endif
