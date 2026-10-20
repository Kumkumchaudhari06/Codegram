

#ifndef CODEDEBUGGER_H
#define CODEDEBUGGER_H

#include <string>
#include <vector>
#include "IncidentAnalyzer/Shared/Evidence.h"

using namespace std;

class CodeDebugger
{
public:
    vector<Evidence> analyzeCode(const string& code,
                                 const string& error);
};

#endif