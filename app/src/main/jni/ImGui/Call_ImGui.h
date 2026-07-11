#pragma once

#include "Font.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"

#include <ctime>
#include <sstream>
#include <iomanip>
#include <string>

using namespace std;

string GetCurrentTime() {
    time_t currentTime = time(nullptr);
    tm* timeInfo = localtime(&currentTime);

    ostringstream oss;
    oss << put_time(timeInfo, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

int ConvertDateTimeToInt(const char* datetimeString) {
    tm timeInfo = {};
    istringstream ss(datetimeString);
    int temp;

    ss >> timeInfo.tm_year;
    ss.ignore(); // Bỏ qua dấu "-"
    ss >> timeInfo.tm_mon;
    ss.ignore(); // Bỏ qua dấu "-"
    ss >> timeInfo.tm_mday;
    ss.ignore(); // Bỏ qua dấu " "
    ss >> timeInfo.tm_hour;
    ss.ignore(); // Bỏ qua dấu ":"
    ss >> timeInfo.tm_min;
    ss.ignore(); // Bỏ qua dấu ":"
    ss >> timeInfo.tm_sec;

    timeInfo.tm_year -= 1900;
    timeInfo.tm_mon -= 1;

    time_t timestamp = mktime(&timeInfo);

    int result = static_cast<int>(timestamp);
    return result;
}