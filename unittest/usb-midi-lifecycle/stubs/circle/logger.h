#pragma once
void TestMidiWarning (const char *, unsigned, unsigned);
void TestMidiRecovery (const char *, unsigned);
#define LOGMODULE(name)
#define LOGWARN TestMidiWarning
#define LOGDBG TestMidiRecovery
