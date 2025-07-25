﻿#pragma once

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>

// Add QtCharts headers here for global access
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

// Windows API headers
#include <windows.h>
#include <sstream>
#include <string>

// C++20 specific headers if needed
#include <concepts>
#include <ranges>
#include <span>

// Ensure Qt compatibility
#ifndef __cplusplus
#define __cplusplus 202002L
#endif

// Define MAX_DATA_POINTS if not already defined
#ifndef MAX_DATA_POINTS
#define MAX_DATA_POINTS 1000
#endif

// Make sure QtCharts namespace is accessible without conflicts
namespace QtCharts {}

// Make QtCharts namespace available throughout the application
using namespace QtCharts;
