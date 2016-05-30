#include "spikeProp2double.cc"

int main(int argc, char** argv)
{
  int f;
  string BPFileNames[12];
  for(f=0; f<10; f++)
    BPFileNames[f] = "data/10-4p5-5Poisson" + stringify(f) + ".pat";

  Network* net;

  for(f=0; f<10; f++)
  {
    net = new Network(BPFileNames[f], 20, -0.01, 0.1, 0.0001, 100);
//  net.printLayout();
//  net.printPatterns();
//  net.printWeights();
    net->trainAllPatterns(50);
    net->testAllPatterns();
    delete net;
  }
  return(0);
}
