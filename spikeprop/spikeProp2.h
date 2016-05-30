// spikeProp2.h
// bpspiking.h
// Olaf Booij
// Mon Sep 22 13:28:48 CEST 2003
// -
// Wed Jan  7 13:36:29 CET 2004

class Network;
class NeuronLayer;
class Neuron;
class InputNeuron;
class HiddenNeuron;
class OutputNeuron;
class Firing;
class WeightedFiring;
class FiringPlus;
class SpikeTrain;
class Synapse;
class Pattern;

// Firing
class Firing
{
 public:
  Firing(double t);
  double getSpikeTime();
 private:
  double spikeTime;
};

// WeightedFiring
class WeightedFiring : public Firing
{
 public:
  WeightedFiring(double t, double w);
  double getWeight ();
 private:
  double weight;
};

// FiringPlus
class FiringPlus : public Firing
{
 public:
  FiringPlus(double t);
  ~FiringPlus();
  double getError();
  double getDelta();
  void setError(double e);
  void addError(double aE);
  void setDelta(double d);
 private:
  double errorToSpike;
  double delta;
};

// overloading the < operator for WeightedFiring 
bool operator<(WeightedFiring a, WeightedFiring b);

// SpikeTrain
class SpikeTrain : public vector<FiringPlus>
{
 public:
  SpikeTrain();
  void addSpike(double spikeTime);
  void printSpikeTimes();
};

// Network
class Network
{
 public:
  // constructors:
  Network(string BPFileName, string extraKey="", bool bat = false, double m=16, int b = 16.0, double iLB = -1, double iUB = 2, double lR = 0.01, double sE = 1.0);
  // destructors:
  ~Network();
  // getters:
  string getKey();
  double getError();
  // setters:
  // prints:
  void printWeights();
  void printLayout();
  void printPatterns();
  void printSpikeTrains();
  void printErrors();
  // rest:
  void propagate();
  void errorPropagate();
  void changeWeights();
  double trainPattern(Pattern pattern);
  double testPattern(Pattern pattern);
  void trainAllPatterns(int nrOfCycles = 500);
  void testAllPatterns();
 private:
  bool readBPFile(string BPFileName);
  void readPatternStream(ifstream *patternsStream, vector<Pattern> *patterns);
  void initLog();
  void connectFeedForward();
  void clamp(Pattern pattern);
  void reset();
  Neuron* getNeuron(string neuronKey);
  bool batchMode; // batch-mode, if true: training is in batch-mode
  double mu; // maxdelta - mindelta miliseconds 
  double beta; // nr of deltaas
  double initUpperBound;
  double initLowerBound;
  double learningRate;
  double stopError;
  int nrOfTimeSteps;
  Logging log;
  string key;
  vector<NeuronLayer> *neuronLayers;
  vector<Pattern> *trainPatterns;
  vector<Pattern> *testPatterns;
};

// NeuronLayer
class NeuronLayer : public map<string, Neuron*>
{
 public:
  // constructors:
  NeuronLayer(string k);
  // destructors:
  ~NeuronLayer();
  // getters: 
  string getKey();
  NeuronLayer* getPreLayer();
  Neuron* getNeuron(string neuronKey);
  double getError();
  //setters:
  void setLog(Logging* l);
  // printers:
  void printWeights();
  void printSpikeTrains();
  void printErrors();
  void logPotential();
  // rest:
  void connectWithPreLayer(NeuronLayer* pL, double delay = 1.0);
//  void connectWithPreLayer(vector<NeuronLayer>::iterator pL, double delay = 1.0);
  void propagate();
  void errorPropagate(); 
  void changeWeights();
  void reset();
 private:
  string key;
  NeuronLayer* preLayer;
  Logging* log;
};

// Neuron
class Neuron
{
 public:
   // constructors:
  Neuron (string k);
   // destructors:
  virtual ~Neuron();
   // getters:
  string getKey();
  SpikeTrain* getSpikeTrain();
   // setters:
  void setThreshold(double tH);
  void setTauM(double tM);
  void setTauS(double tS);
  void setTauR(double tR);
  void setLog(Logging* l);
  void setInitLowerBound(double iLB);
  void setInitUpperBound(double iUB);
  void setLearningRate(double lR);
   // prints:
  void printKey();
  void printWeights();
  void printSpikeTrain();
  void printError();
  void logPotential();
   // rest:
  void connectWithPreNeuron(Neuron *preNeuron, double delay = 1.0);
  void connectWithPostSynapse(Synapse *postSynapse);
  void connectWithPreSynapse(Synapse *preSynapse);
  virtual void propagate();
  virtual void clamp(SpikeTrain* sT);
  void fire();
  void addIncomingSpike(WeightedFiring incomingSpike);
  virtual void reset();
// errorfunctions:
  double getSquaredError();
  void errorPropagate(); 
  void changeWeights();
  void signalSpikeError(SpikeTrain::iterator spikeIt, double sE);
  int neuronType; // should be protected/private
 protected:
  virtual void calcSpikeError(SpikeTrain::iterator spikeIt);
  set<Synapse*> *preSynapses;
  set<Synapse*> *postSynapses;
  SpikeTrain* spikeTrain;
  int epsilonMax;
  double normalizeEpsilon;
  double tauR, tauM, tauS;
  double tauRe, tauMe, tauSe;   
 private:
  // functions:
  void calculatePreSynapticFiring();
  void calcDelta();
  void sendError(set<Synapse*>::iterator synapseIter);
  // objects:
  priority_queue<WeightedFiring> *incomingSpikes;
  Logging *log;
  // parameters:
  string key;
  double threshold;
  double initLowerBound;
  double initUpperBound;
  double learningRate;
  // variables:
  double uM, uR, uS;
};

// InputNeuron
class InputNeuron : public Neuron
{
 public:
  InputNeuron(string key, int neuronT = 0);
  virtual void clamp(SpikeTrain* sT);
  virtual void propagate();
  virtual void reset();
 private: 
  SpikeTrain::iterator current;
};

// HiddenNeuron
class HiddenNeuron : public Neuron
{
 public:
  HiddenNeuron(string key, int neuronT = 0);
 private:
  virtual void calcSpikeError(SpikeTrain::iterator spikeIt);
};

// OutputNeuron
class OutputNeuron : public Neuron
{
 public:
  OutputNeuron(string key);
  virtual void clamp(SpikeTrain* dST);
  virtual void propagate();
  virtual void reset();
 private:
  virtual void calcSpikeError(SpikeTrain::iterator spikeIt);
  SpikeTrain* desiredSpikeTrain;
};


// Synapse
class Synapse
{
 public:
  // constructors:
//  Synapse(Neuron* prN, Neuron* poN);
  Synapse(Neuron* prN, Neuron* poN, double initLowerBound, double initUpperBound, double lR, double d = 1, int synapseT = 0);
//  Synapse(Neuron* prN, Neuron* poN, double d = 1, double initLowerBound = -0.5, double initUpperBound = 1.0);
  // destructors:
  ~Synapse();
  // getters:
  string getKey();
  double getWeight();
  double getDelay(); 
  double getError();
  Neuron* getPreNeuron();
  Neuron* getPostNeuron();
  // setters:
  void setLowerBound(double lB);
  void setUpperBound(double uB);
  void setLog(Logging* l);
  // prints:
  void printWeight();
  // rest:
  void fire();
  void reset();
  void signalWeightError(double weightError);
  void changeWeight();
 private:
  void initWeight(double initLowerBound, double initUpperBound);
  string key;
  Neuron* preNeuron;
  Neuron* postNeuron;
  Logging *log;
  double weight;
  double delay;
  double lowerBound;
  double upperBound;
  double error;
  double learningRate;
};

class Pattern : public map<string, SpikeTrain*>
{
 public:
  Pattern();
  ~Pattern();
  Pattern(string k);
  string getKey();
  void print();
 private:
  string key;
};
