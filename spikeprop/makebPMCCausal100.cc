
using namespace std;
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <string>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include "/home/obooij/study/neuraal/spikeProp2/spikeProp2.cc"

class RandomSpikeTrain : public SpikeTrain
{
 public:
  void fillWithPoisson(double rate, int tmax);
  void causalize();
  void printNrOfSpikes();
  void makeNoisySpikeTimes(double sigma, RandomSpikeTrain* noisyST);
  int tmax;
 private:
};

void RandomSpikeTrain::fillWithPoisson(double rate, int tm)
{
  tmax = tm;
  double randomnummer;
  double P;
  double dt = 0.01; // dt is 1 millisecond 
  double t; // time in milliseconds
  for(t = 0; t < tmax; t += dt)
  {
    randomnummer = (double) rand()/RAND_MAX;
    P = (double) rate *  (double) dt;
    if (P > randomnummer)
      addSpike(t);
  }
}

void RandomSpikeTrain::causalize()
{
  iterator firingIt;
  double spikeTimes[100]; //arrays are no good
  double spikeTimesCaus[100]; //arrays are no good
  int nrOfSpikes;
  double previousSpikeTime = -1;
  int i = 0;
  int i2;
  double onthoud = -1;
  int winner;
  double mini;
  for(firingIt=begin(); firingIt != end(); firingIt++)
  {
    spikeTimes[i] = firingIt->getSpikeTime();
    i++;
  }
  nrOfSpikes = i;
  clear(); //memory leak
  //sort:
  for(i2=0; i2<nrOfSpikes; i2++)
  {
    mini = 100;
    for(i=0; i<nrOfSpikes; i++)
    {
      if (spikeTimes[i] < mini)
      {
        mini = spikeTimes[i];
        winner = i;
      }
    }
    if(mini <= onthoud)
      mini = onthoud + 1;
    addSpike(mini);
    onthoud = mini;
    spikeTimes[winner] = 100;
  }
}

void RandomSpikeTrain::printNrOfSpikes()
{
  cout << size() << " ";
}

void RandomSpikeTrain::makeNoisySpikeTimes(double sigma, RandomSpikeTrain* noisyST)
{
  const double pi = 3.141592654;
  iterator firingIt;
  double exactSpikeTime;
  double noisySpikeTime;
  double randomnummer;
  noisyST->clear(); //memory leak
  for(firingIt=begin(); firingIt != end(); firingIt++)
  {
    exactSpikeTime = firingIt->getSpikeTime();
    do
    {
      noisySpikeTime = ((double) rand()/RAND_MAX) * tmax;
    } while ((double) 1/(sigma * sqrt(2*pi)) * exp(-0.5 * pow(((noisySpikeTime-exactSpikeTime)/sigma), 2)) < (double) rand()/RAND_MAX);
    noisyST->addSpike(noisySpikeTime);
  }
  noisyST->tmax = tmax;
}

int main(void)
{
  const double pi = 3.141592654;
  int i,h,p,c,c2,nrOf;

  // 1 output neuron (2 klasses)
  int nrOfInputs = 2;
  int nrOfHiddens = 5;
  int nrOfClasses = 2;
  int nrOfTrPatsPClass = 5;
  int nrOfTePatsPClass = 5;
  int inputTimeWindow = 30;
  double spikeRate = 0.1; // 100 Hz
  double noiseRate = 2.0;
  
  RandomSpikeTrain sT[nrOfInputs][nrOfClasses];  
  RandomSpikeTrain noisySpikeTrain;
  
  // generate random spiketrains:
  srand(time(NULL));
  for(h = 0; h < nrOfInputs; h++)
    for(c = 0; c < nrOfClasses; c++)
      sT[h][c].fillWithPoisson(spikeRate, inputTimeWindow);

  //write file:
  cout << nrOfInputs+1;
  for(h = 0; h <nrOfInputs+1; h++)
    cout << " I0-" << h;
  cout << endl;
  cout << "1 " << nrOfHiddens; // hiddens
  for(h = 0; h < nrOfHiddens-1; h++)
    cout << " 1"; // almost all excit
  cout << " -1" << endl; // one inhibit
  cout << 1;
  for(c = 0; c < 1; c++)
    cout << " J2-" << c;
  cout << endl;
  for(i=0; i < 2; i++)  
  {
    if (i==0)
      nrOf = nrOfTrPatsPClass;
    else
      nrOf = nrOfTePatsPClass;
    cout << nrOf*nrOfClasses << endl;
    for(c=0; c < nrOfClasses; c++)
    {
      for(p=0; p < nrOf; p++)
      {
        if (i==0)
          cout << "PTr" << c << "-" << p << endl;
        else
          cout << "PTe" << c << "-" << p << endl;
        cout << (nrOfInputs + 1 +1) << endl;
        for(h = 0; h <nrOfInputs; h++)
        {

          sT[h][c].makeNoisySpikeTimes(noiseRate, &noisySpikeTrain);
          noisySpikeTrain.causalize();
          cout << "I0-" << h << " ";
          sT[h][c].printNrOfSpikes();
          noisySpikeTrain.printSpikeTimes();
        }
        cout << "I0-" << h << " 1 0" << endl; //bias
        cout << "J2-" << 0 << " 1  ";
        if (c)
          cout << inputTimeWindow+1 << endl; // early
        else
          cout << inputTimeWindow+6 << endl; // late
      }
    }
  }
  /*
  RandomSpikeTrain r;
  r.push_back(*(new Firing(6)));
  r.push_back(*(new Firing(4)));
  r.push_back(*(new Firing(3)));
  r.push_back(*(new Firing(3)));
  r.push_back(*(new Firing(3)));
  r.push_back(*(new Firing(10)));
  r.push_back(*(new Firing(10)));
  r.push_back(*(new Firing(10)));
  r.tmax = 16;
  r.printSpikeTimes();
  r.causalize();
  r.printSpikeTimes();
  r.makeNoisySpikeTimes(4.0, &noisySpikeTrain);
  noisySpikeTrain.printSpikeTimes();
  noisySpikeTrain.causalize();
  noisySpikeTrain.printSpikeTimes();
  */

  return(0);  
}
