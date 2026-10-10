
#include "CodeDebugger.h"

vector<Evidence> CodeDebugger::analyzeCode(
    const string& code, const string& error)
{
    vector<Evidence> evidence;

    if (error.find("undefined reference") != string::npos)
    {
        evidence.push_back(Evidence(
            "Function or variable definition may be missing",
            "Compiler error", 8));
    }

    if (error.find("expected") != string::npos)
    {
        evidence.push_back(Evidence(
            "Possible syntax error, such as a missing semicolon",
            "Compiler error", 7));
    }

    if (code.find("NULL") != string::npos)
    {
        evidence.push_back(Evidence(
            "Code contains a NULL reference that needs checking",
            "Source code", 3));
    }

    return evidence;
}