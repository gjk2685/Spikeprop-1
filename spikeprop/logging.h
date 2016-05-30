// logging.h
//
// Wed Oct  8 13:00:46 CEST 2003
//


class Logging 
{
 public:
  Logging();
  ~Logging();
  bool assignFile(string logKey, string fileName);
  bool log(string logKey, string logData);
  void setHeader(string h);
 private:
  map<string, ofstream*> logStreams;
  string header;
};
