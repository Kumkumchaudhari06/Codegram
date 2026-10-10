
#ifndef EVIDENCE_H
#define EVIDENCE_H

#include <string>
using namespace std;

class Evidence
{
public:
    string description;
    string source;
    int score;

    Evidence(string d, string s, int sc)
    {
        description = d;
        source = s;
        score = sc;
    }
};

#endif