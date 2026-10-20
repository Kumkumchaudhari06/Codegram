
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include "RootCauseFinder/CodeDebugger/CodeDebugger.h"
#include "RootCauseFinder/CodeDebugger/IncidentAnalyzer/IncidentAnalyzer.h"

using namespace std;

int main()
{
    CodeDebugger debugger;
    IncidentAnalyzer analyzer;

    // Sample code and compiler error
    string code = "int main() { return 0; }";
    string error = "undefined reference to function";

    vector<Evidence> codeEvidence =
        debugger.analyzeCode(code, error);

    // Rank code evidence by score (highest first)
    sort(codeEvidence.begin(), codeEvidence.end(),
         [](const Evidence& a, const Evidence& b)
         {
             return a.score > b.score;
         });

    cout << "=== CODE ROOT CAUSE FINDER ===\n";

    for (int i = 0; i < codeEvidence.size(); i++)
    {
        cout << "Rank: " << i + 1 << endl;
        cout << "Possible cause: "
             << codeEvidence[i].description << endl;
        cout << "Source: "
             << codeEvidence[i].source << endl;
        cout << "Score: "
             << codeEvidence[i].score << endl << endl;
    }

    // Sample incident and system logs
    vector<string> logs = {
        "Database connection timeout",
        "Request failed"
    };

    vector<Evidence> incidentEvidence =
        analyzer.analyzeIncident("Database issue", logs);

    // Rank incident evidence by score (highest first)
    sort(incidentEvidence.begin(), incidentEvidence.end(),
         [](const Evidence& a, const Evidence& b)
         {
             return a.score > b.score;
         });

    cout << "=== INCIDENT ROOT CAUSE FINDER ===\n";

    for (int i = 0; i < incidentEvidence.size(); i++)
    {
        cout << "Rank: " << i + 1 << endl;
        cout << "Possible cause: "
             << incidentEvidence[i].description << endl;
        cout << "Source: "
             << incidentEvidence[i].source << endl;
        cout << "Score: "
             << incidentEvidence[i].score << endl << endl;
    }

    return 0;
}