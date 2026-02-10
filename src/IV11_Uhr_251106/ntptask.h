#pragma once

void initTime();
void initNtpServer(const String& server);
void initTimeZone(const String& tz);
bool isTimeSynchronized();
tm getNtpTime();