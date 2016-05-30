// logging.cc
//
// Wed Oct  8 13:15:07 CEST 2003
//


#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include "logging.h"

// Logging:
Logging::Logging()
{
  header = "";
}

Logging::~Logging()
{
  map<std::string, ofstream*>::iterator streamIt;

  for(streamIt = logStreams.begin(); streamIt != logStreams.end(); streamIt++)
  {
    cout << "Logging::~Logging : closing stream with logKey " << streamIt->first << " and deleting the stream." << endl;
    streamIt->second->close();
    delete streamIt->second;
  }
}

bool Logging::assignFile(string logKey, string fileName)
{
  if(logStreams.find(logKey) != logStreams.end())
  {
    cerr << "Logging::assignFile : Error: assigning logKey " << logKey << " that is already assigned." << endl;
    return(0);
  }
  ofstream* newLogStream = new ofstream(fileName.c_str());
  
  if (!newLogStream->is_open())
  {
    cerr << "Logging::assignFile : Error: Error opening \"" << fileName << "\"." << endl;
    return(0);
  }
      
  logStreams.insert(make_pair(logKey, newLogStream));
  cout << "Logging::assignFile : opened \"" << fileName << "\" with logKey " << logKey << " and inserted it in the logmap." << endl;
  return(1);
} 

bool Logging::log(string logKey, string logData)
{
  map<string, ofstream*>::iterator streamIt = logStreams.find(logKey);
  
  if(streamIt == logStreams.end())
  {
    cerr << "Logging:log : Warning: logging to logKey " << logKey << " that is not assigned." << endl;
    return(0);
  }
  (*streamIt->second) << header << logData << endl;
  return(1);
}

void Logging::setHeader(string h)
{
  header = h;
}

/*
int main(void)
{
  Logging logObj;
  
  logObj.assignFile("sleutel1", "tester.log");
  logObj.assignFile("sleutel1", "tester2.log");
  logObj.assignFile("sleutel2", "tester2.log");
  logObj.log("sleutel2", "mijn eerste log");
  logObj.log("sleutel2", "mijn tweede log");
  logObj.log("sleutel1", "mijn derde log");
  logObj.log("sleutel3", "mijn niet goede log");
}
*/
