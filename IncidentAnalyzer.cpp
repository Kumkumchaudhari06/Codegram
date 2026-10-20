#include "IncidentAnalyzer.h"

vector<Evidence> IncidentAnalyzer::analyzeIncident(
    const string& incident,
    const vector<string>& logs)
{
    vector<Evidence> evidence;

    if (incident.find("database") != string::npos ||
        incident.find("Database") != string::npos)
    {
        evidence.push_back(Evidence(
            "Database may be related to the incident",
            "Incident description", 5));
    }

    for (const string& log : logs)
    {
        if (log.find("timeout") != string::npos ||
            log.find("Timeout") != string::npos)
        {
            evidence.push_back(Evidence(
                "A timeout was found in the system logs",
                log, 8));
        }

        if (log.find("failed") != string::npos ||
            log.find("Failed") != string::npos)
        {
            evidence.push_back(Evidence(
                "A failure was reported in the system logs",
                log, 6));
        }
    }

    return evidence;
}
