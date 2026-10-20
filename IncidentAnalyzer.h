
#ifndef INCIDENTANALYZER_H
#define INCIDENTANALYZER_H

#include <string>
#include <vector>
#include "Shared/Evidence.h"

using namespace std;

class IncidentAnalyzer
{
public:
    vector<Evidence> analyzeIncident(
        const string& incident,
        const vector<string>& logs);
};

#endif