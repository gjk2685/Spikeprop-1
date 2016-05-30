// spikeProp2ef2.cc
// spikeProp2double.cc
// spikeProp2.cc
// bpspiking.cc
// Olaf Booij
// Mon Sep 22 13:28:48 CEST 2003
// -
// Wed Jan  7 13:36:29 CET 2004

using namespace std;
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <algorithm>
#include <string>
#include <fstream>
#include <cmath>
#include <ctime>
#include "convert.h"
#include "logging.cc"
#include "spikeProp2.h"

//global!
double NOW = 0;
double TIMESTEP = 0.1; // oh my god: another global

// Network
// constructors:
Network::Network(string BPFileName, string extraKey, bool bat, double m, int b, double iLB, double iUB, double lR, double sE)
{
  int charPosition;
  key = BPFileName;
  charPosition = key.rfind("/", key.length() - 1);
  key.erase(0, charPosition + 1);
  charPosition = key.find(".", 0);
  key.erase(charPosition, key.length() - charPosition);
  key += extraKey;

  batchMode = bat;
  mu = m;
  beta = b;
  initLowerBound = iLB;
  initUpperBound = iUB;
  learningRate = lR;
  stopError = sE;

  srand(time(0));    //given the parameter of the seed to make the rand function output different numbers each time
  neuronLayers = new vector<NeuronLayer>;
  trainPatterns = new vector<Pattern>;
  testPatterns = new vector<Pattern>;
  readBPFile(BPFileName);
  nrOfTimeSteps = static_cast<int>(floor(40 / TIMESTEP)); // should be depending on file-input
  initLog();
}

Network::~Network()
{
  NeuronLayer::iterator neuronIt;
  vector<NeuronLayer>::iterator it;
  vector<Pattern>::iterator patternIt;
  Pattern::iterator spikeTrainIt;
  //delete neurons and neuronlayers:
  for(it = neuronLayers->begin(); it != neuronLayers->end(); it++)
  {
    for(neuronIt=it->begin(); neuronIt != it->end(); neuronIt++)
      delete neuronIt->second;
    it->clear();
  }
  neuronLayers->clear();
  delete neuronLayers;

  //delete train- and test-patterns:
  for(patternIt = trainPatterns->begin(); patternIt != trainPatterns->end(); patternIt++)
  {
    for(spikeTrainIt = patternIt->begin(); spikeTrainIt != patternIt->end(); spikeTrainIt++)
      if (spikeTrainIt->second)
        delete spikeTrainIt->second;
  }
  for(patternIt = testPatterns->begin(); patternIt != testPatterns->end(); patternIt++)
    for(spikeTrainIt = patternIt->begin(); spikeTrainIt != patternIt->end(); spikeTrainIt++)
      if (spikeTrainIt->second)
        delete spikeTrainIt->second;
  trainPatterns->clear();
  testPatterns->clear();
  delete trainPatterns;
  delete testPatterns;
}

string Network::getKey()
{
  return(key);
}

double Network::getError()
{
  return(neuronLayers->back().getError());
}

void Network::printWeights()
{
  vector<NeuronLayer>::iterator it;
  for(it = neuronLayers->begin(); it != neuronLayers->end(); it++)
    it->printWeights();
}

void Network::printSpikeTrains()
{
  vector<NeuronLayer>::iterator it;
  for(it = neuronLayers->begin(); it != neuronLayers->end(); it++)
    it->printSpikeTrains();
}

void Network::printLayout()
{
  vector<NeuronLayer>::iterator neuronLayerIt;
  vector<Pattern>::iterator patternIt;

  cout << "Name of Network : " << getKey() << endl;
  cout << neuronLayers->size() << " layers of neurons" << endl;
  for(neuronLayerIt = neuronLayers->begin(); neuronLayerIt != neuronLayers->end(); neuronLayerIt++)
  {
    cout << "    " << neuronLayerIt->getKey();
    cout << " : " << neuronLayerIt->size() << " neuron(s)";
    if(neuronLayerIt->getPreLayer())
      cout << "   connected with: " << neuronLayerIt->getPreLayer()->getKey() << endl;
    else 
      cout << endl;
  }

  cout << trainPatterns->size() << " train-pattern(s) "<< endl;
  for(patternIt = trainPatterns->begin(); patternIt != trainPatterns->end(); patternIt++)
    cout << "    "<< patternIt->getKey() << endl;

  cout << testPatterns->size() << " test-pattern(s) "<< endl;
  for(patternIt = testPatterns->begin(); patternIt != testPatterns->end(); patternIt++)
    cout << "    "<< patternIt->getKey() << endl;
}

void Network::printPatterns()
{
  vector<Pattern>::iterator patternIt;
  cout << trainPatterns->size() << " train-pattern(s) "<< endl;
  for(patternIt = trainPatterns->begin(); patternIt != trainPatterns->end(); patternIt++)
    patternIt->print();
  cout << testPatterns->size() << " test-pattern(s) "<< endl;
  for(patternIt = testPatterns->begin(); patternIt != testPatterns->end(); patternIt++)
    patternIt->print();
}

void Network::printErrors()
{
  vector<NeuronLayer>::iterator it;
  for(it = neuronLayers->begin(), it++; it != neuronLayers->end(); it++)
    it->printErrors();
}

void Network::propagate()
{
  vector<NeuronLayer>::iterator neuronLayerIt;
  for(neuronLayerIt = neuronLayers->begin(); neuronLayerIt != neuronLayers->end(); neuronLayerIt++)
    neuronLayerIt->propagate();
  NOW += TIMESTEP; // global
}

void Network::errorPropagate()
{
  vector<NeuronLayer>::reverse_iterator neuronLayerReIt;
  vector<NeuronLayer>::reverse_iterator neuronLayerReIt2;
  
  // propagating the error backwards, so start with the output-layer
  // without inputLayer...
  neuronLayerReIt = neuronLayers->rbegin();
  neuronLayerReIt2 = neuronLayers->rbegin();
  neuronLayerReIt2++;
  while (neuronLayerReIt2 != neuronLayers->rend())
  {
    neuronLayerReIt->errorPropagate();
    neuronLayerReIt = neuronLayerReIt2;
    neuronLayerReIt2++;
  }

  //for(neuronLayerReIt = neuronLayers->rbegin(); neuronLayerReIt != neuronLayers->rend(); neuronLayerReIt++)
  //{
  //  neuronLayerReIt->errorPropagate();
  //}
}

void Network::changeWeights()
{
  vector<NeuronLayer>::iterator neuronLayerIt;
  for(neuronLayerIt = neuronLayers->begin(); neuronLayerIt != neuronLayers->end(); neuronLayerIt++)
    neuronLayerIt->changeWeights();
}

double Network::trainPattern(Pattern pattern)
{
  int i;

  reset();
//  cout << "clamping and propagating pattern " << pattern.getKey() << endl;
  clamp(pattern);
  for(i=0; i<nrOfTimeSteps; i++)
    propagate();
//  cout << "backpropagating error" << endl;
  errorPropagate();
  if(!batchMode)
    changeWeights();
//  cout << "done" << endl;
  //printSpikeTrains();
  //printErrors();
  // printWeights();
  return(getError());
}

double Network::testPattern(Pattern pattern)
{
  int i;

  reset();
  cout << "clamping pattern " << pattern.getKey() << endl;
  clamp(pattern);
  for(i=0; i<nrOfTimeSteps; i++)
  {
    propagate();
    neuronLayers->back().logPotential();
  }
  errorPropagate();
//  printSpikeTrains();
  printErrors();
  return(getError());
}

void Network::trainAllPatterns(int nrOfCycles)
{
  int c;
  double error = 99999.0;
  double lowestError = error;
  int lowestErrorCycle;
  vector<Pattern>::iterator patternIt;
  if (trainPatterns->empty())
    cout << "Network::trainAllPatterns: Error: no train-patterns loaded" << endl;
  else 
    for(c=0; c < nrOfCycles && error > stopError; c++) {
      error = 0.0;
      random_shuffle(trainPatterns->begin(), trainPatterns->end());
      for(patternIt = trainPatterns->begin(); patternIt != trainPatterns->end(); patternIt++)
      {
        log.setHeader(stringify(c) + " " + patternIt->getKey() + "  ");
        error += trainPattern(*patternIt);
      }
      if(batchMode)
        if(error > stopError) // to prevent changing after stopcriteria is met
          changeWeights();
      if(error < lowestError)
      {
        lowestError = error;
        lowestErrorCycle = c;
        cout << "Cycle : " << c << ",  Summed output error = " << error << endl;
      }
//      cout << "Cycle : " << c << ",  Summed output error = " << error << " Lowest error = " << lowestError << " cycle= " << lowestErrorCycle << endl;
    }
}

void Network::testAllPatterns()
{
  int c;
  vector<Pattern> *patterns;
  vector<Pattern>::iterator patternIt;
  if (testPatterns->empty())
  {
    cout << "Network::testAllPatterns: Error: no test-patterns loaded, using train-patterns." << endl;
    patterns = trainPatterns;
  }
  else 
    patterns = testPatterns;
  for(patternIt = patterns->begin(); patternIt != patterns->end(); patternIt++)
  {
    log.setHeader(stringify(c) + " " + patternIt->getKey() + "  ");
    testPattern(*patternIt);
  }
}

bool Network::readBPFile(string BPFileName)
{
  int nrOfInputs;
  int nrOfOutputs;
  int nrOfHiddenLayers;
  int nrOfNeurons;
  int i, l;
  int neuronT;
  vector<NeuronLayer>::iterator layerIt;
  NeuronLayer::iterator neuronIt;

  string neuronName;
  string layerName;
  ifstream patternsStream(BPFileName.c_str());
  if(patternsStream == NULL) {
    cout << " Network::readBPFile Error: file \"" << BPFileName << "\" is unreadable." << endl;
    return(0);
  }

  // reading and making inputneurons:
  patternsStream >> nrOfInputs;
  neuronLayers->push_back(*(new NeuronLayer("I")));
  for(i=0; i<nrOfInputs; i++)
  {
    patternsStream >> neuronName;
    neuronLayers->front().insert(make_pair(neuronName, new InputNeuron(neuronName, 0)));
  }

  // reading and making hiddenlayers:
  patternsStream >> nrOfHiddenLayers;
  for(l=0; l<nrOfHiddenLayers; l++) {
    patternsStream >> nrOfNeurons;
    layerName = "H" + stringify(l+1);
    neuronLayers->push_back(*(new NeuronLayer(layerName)));
    for(i=0; i<nrOfNeurons; i++)
    {
      neuronName = layerName + "-" + stringify(i);
      patternsStream >> neuronT; // extra for weight-fix
      neuronLayers->back().insert(make_pair(neuronName, new HiddenNeuron(neuronName, neuronT)));
    }
  }

  // reading and making outputneurons:
  patternsStream >> nrOfOutputs;
  neuronLayers->push_back(*(new NeuronLayer("J")));
  for(i=0; i<nrOfOutputs; i++)
  {
    patternsStream >> neuronName;
    neuronLayers->back().insert(make_pair(neuronName, new OutputNeuron(neuronName)));
  }

  // reading and making trainingpatterns
  readPatternStream(&patternsStream, trainPatterns);
  readPatternStream(&patternsStream, testPatterns);
  
  patternsStream.close();

  // setting local neuron parameters:
  for(layerIt = neuronLayers->begin(); layerIt != neuronLayers->end(); layerIt++)
    for(neuronIt=layerIt->begin(); neuronIt != layerIt->end(); neuronIt++)
    {
      neuronIt->second->setLearningRate(learningRate);
      neuronIt->second->setInitLowerBound(initLowerBound);
      neuronIt->second->setInitUpperBound(initUpperBound);
    }
  connectFeedForward();
  return(1);
}

void Network::readPatternStream(ifstream* patternsStream, vector<Pattern> *patterns)
{
  int p, i, s;
  int nrOfPatterns;
  int nrOfNeurons;
  int nrOfSpikes;
  int spikeTime;
  string neuronName;
  string patternName;
  *patternsStream >> nrOfPatterns;
  for(p=0; p<nrOfPatterns; p++)
  {
    *patternsStream >> patternName;
    patterns->push_back(*(new Pattern(patternName)));
    *patternsStream >> nrOfNeurons;
    for(i=0; i<nrOfNeurons; i++) {
      *patternsStream >> neuronName;
      patterns->back().insert(make_pair(neuronName, new SpikeTrain()));
      *patternsStream >> nrOfSpikes;
      for(s=0; s<nrOfSpikes; s++) {
        *patternsStream >> spikeTime;
        patterns->back().find(neuronName)->second->addSpike(spikeTime); //push_back(*(new Firing(spikeTime)));
      }
    }
  }
}
void Network::initLog()
{
  vector<NeuronLayer>::iterator layerIt;
  log.assignFile("Firing", "log/Firing.log");
  log.assignFile("Potential", ("log/" + key + "Potential.log"));
  log.assignFile("Error", ("log/" + key + "Error.log"));
  log.assignFile("SquaredNeuronError", "log/SquaredNeuronError.log");
  log.assignFile("Weight", "log/Weight.log");
  for(layerIt = neuronLayers->begin(); layerIt != neuronLayers->end(); layerIt++)
    layerIt->setLog(&log);
}

void Network::connectFeedForward()
{
  double delay;
  vector<NeuronLayer>::iterator layerIterPre;
  vector<NeuronLayer>::iterator layerIterPost;
  NeuronLayer* preLayerPtr;

  for(delay = (double)(mu/beta); delay <= mu; delay += (double)(mu/beta))
    for(layerIterPre = neuronLayers->begin(), layerIterPost = neuronLayers->begin(), layerIterPost++; layerIterPost != neuronLayers->end(); layerIterPre++, layerIterPost++)
    {
      preLayerPtr = &(*layerIterPre); // AAAH: what i have is iterator, what i want is pointer
      layerIterPost->connectWithPreLayer(preLayerPtr, delay);
    }
}

void Network::clamp(Pattern pattern)
{
  map<string, SpikeTrain*>::iterator neuronIter; 
  
  for(neuronIter = pattern.begin(); neuronIter != pattern.end(); neuronIter++)
    getNeuron(neuronIter->first)->clamp(neuronIter->second);
}

void Network::reset()
{
  NOW = 0.0; // global
  vector<NeuronLayer>::iterator neuronLayerIt;
  for(neuronLayerIt = neuronLayers->begin(); neuronLayerIt != neuronLayers->end(); neuronLayerIt++)
    neuronLayerIt->reset();
}

Neuron* Network::getNeuron(string neuronKey)
{
  vector<NeuronLayer>::iterator layerIter = neuronLayers->begin(); 
  Neuron* neuron;
  do {
    neuron = layerIter->getNeuron(neuronKey);
    layerIter++;
  }
  while (neuron == NULL && layerIter != neuronLayers->end()); 
  return(neuron);
}


// NeuronLayer:
NeuronLayer::NeuronLayer(string k): key(k)
{
  preLayer = NULL;
}

NeuronLayer::~NeuronLayer()
{
//  cout << "dak " << key << "hier ben" << endl;
//  clear();
}

string NeuronLayer::getKey()
{
  return(key);
}

NeuronLayer* NeuronLayer::getPreLayer()
{
  return(preLayer);
}

Neuron* NeuronLayer::getNeuron(string neuronKey)
{
  iterator neuronIter = find(neuronKey);
  if (neuronIter == end())
    return(NULL);
  else
    return(neuronIter->second);
}

double NeuronLayer::getError()
{
  double error = 0.0;
  iterator neuronIt;
  for(neuronIt=begin(); neuronIt != end(); neuronIt++)
    error += neuronIt->second->getSquaredError();
  return(error);
}

void NeuronLayer::printWeights()
{
  iterator neuronIt;
  cout << getKey() << endl;
  for(neuronIt=begin(); neuronIt != end(); neuronIt++)
    neuronIt->second->printWeights();
}

void NeuronLayer::printSpikeTrains()
{
  iterator neuronIt;
  cout << getKey() << endl;
  for(neuronIt=begin(); neuronIt != end(); neuronIt++)
    neuronIt->second->printSpikeTrain();
}

void NeuronLayer::printErrors()
{
  iterator neuronIt;
  for(neuronIt=begin(); neuronIt != end(); neuronIt++)
    neuronIt->second->printError();
}

void NeuronLayer::logPotential()
{
  iterator neuronIt;
  for(neuronIt=begin(); neuronIt != end(); neuronIt++)
    neuronIt->second->logPotential();
}

void NeuronLayer::connectWithPreLayer(NeuronLayer* pL, double delay)
//void NeuronLayer::connectWithPreLayer(vector<NeuronLayer>::iterator 0NeuronLayer* pL, double delay)
{
  map<string, Neuron*>::iterator layerIterPre;
  iterator layerIterPost;
  preLayer = pL;
  
  for(layerIterPost = begin(); layerIterPost != end(); layerIterPost++)
    for(layerIterPre = preLayer->begin(); layerIterPre != preLayer->end(); layerIterPre++)
      layerIterPost->second->connectWithPreNeuron(layerIterPre->second, delay);
}

void NeuronLayer::propagate()
{
  iterator neuronI;
  for(neuronI = begin(); neuronI != end(); neuronI++)
    neuronI->second->propagate();
}

void NeuronLayer::errorPropagate()
{
  iterator neuronI;
  for(neuronI = begin(); neuronI != end(); neuronI++)
    neuronI->second->errorPropagate();
}

void NeuronLayer::changeWeights()
{
  iterator neuronI;
  for(neuronI = begin(); neuronI != end(); neuronI++)
    neuronI->second->changeWeights();
}

void NeuronLayer::reset()
{
  iterator neuronI;
  for(neuronI = begin(); neuronI != end(); neuronI++)
    neuronI->second->reset();
}

void NeuronLayer::setLog(Logging* l)
{
  iterator neuronI;
  log = l;
  for(neuronI = begin(); neuronI != end(); neuronI++)
    neuronI->second->setLog(log);
}


// een onopgeleide jeugd is ook een vorm van staatsschuld

// Neuron
// constructors:
Neuron::Neuron(string k): key(k)
{
  // default parameter settings:
  setThreshold(1.0);
  setTauM(4.0);
  setTauS(2.0);
  setTauR(20.0);
  //epsilonMax = 3;
  normalizeEpsilon = 1.0;
  // make objects:
  preSynapses = new set<Synapse*>;
  postSynapses = new set<Synapse*>;
  incomingSpikes = new priority_queue<WeightedFiring>;
  // initialize variables 
  uR = 0; 
  uM = 0;
  uS = 0;
}

// destructors:
Neuron::~Neuron()
{
  reset();
  set<Synapse*>::iterator synapseIter;
  for(synapseIter = preSynapses->begin(); synapseIter != preSynapses->end(); synapseIter++)
    delete *synapseIter;
  delete preSynapses;
  delete postSynapses;
  while(!incomingSpikes->empty()) { //allready empty thanks to reset()
    incomingSpikes->pop();
  }
  delete incomingSpikes;
}

// public getters:
string Neuron::getKey()
{
  return(key);
}

SpikeTrain* Neuron::getSpikeTrain()
{
  if(!spikeTrain)
    cout << "Neuron::getSpikeTrain: Error : no spikeTrain clamped/present." << endl;
  return(spikeTrain);
}

double Neuron::getSquaredError()
{
  double sQE = 0.0;
  SpikeTrain::iterator spikeIt;

  if (spikeTrain)
    for(spikeIt = spikeTrain->begin(); spikeIt != spikeTrain->end(); spikeIt++)
      sQE += pow(spikeIt->getError(), 2);
  log->log("SquaredNeuronError", key + " " + stringify(sQE));
  return(sQE);
}

// public setters:
void Neuron::setThreshold(double tH)
{
  threshold = tH;
}

void Neuron::setTauM(double tM)
{
  tauM = tM;
  tauMe = 1/ (exp(1/tauM * TIMESTEP));
}

void Neuron::setTauS(double tS)
{
  tauS = tS;
  tauSe = 1/ (exp(1/tauS * TIMESTEP));
}

void Neuron::setTauR(double tR)
{
  tauR = tR;
  tauRe = 1/ (exp(1/tauR * TIMESTEP));
}

void Neuron::setLog(Logging* l)
{
  set<Synapse*>::iterator synapseIter;
  log = l;
  for(synapseIter = preSynapses->begin(); synapseIter != preSynapses->end(); synapseIter++)
    (*synapseIter)->setLog(log);
}

void Neuron::setInitLowerBound(double iLB)
{
  initLowerBound = iLB;
}

void Neuron::setInitUpperBound(double iUB)
{
  initUpperBound = iUB;
}

void Neuron::setLearningRate(double lR)
{
  learningRate = lR;
}

// public prints:
void Neuron::printKey()
{
  cout << getKey() << " ";
}

void Neuron::printWeights()
{
  set<Synapse*>::iterator synapseIter;
  cout << getKey() << ":\n";
  for(synapseIter = preSynapses->begin(); synapseIter != preSynapses->end(); synapseIter++)
    (*synapseIter)->printWeight();
  cout << "\n";
}

void Neuron::printSpikeTrain()
{
  cout << key << " :  ";
  if(!spikeTrain)
    cout << "Neuron::printSpikeTrain: Error : no spikeTrain clamped/present." << endl;
  else
    spikeTrain->printSpikeTimes();
}

void Neuron::printError()
{
  cout << key << " :  " << "Squared Error = " << getSquaredError() << endl; 
}

void Neuron::logPotential()
{
  log->log("Potential", key + " " + stringify(NOW) + " " + stringify(uM + uR + uS));
}

// public rest:
void Neuron::connectWithPreNeuron(Neuron *preNeuron, double delay)
{
  Synapse *synapse = new Synapse(preNeuron, this, initLowerBound, initUpperBound, learningRate, delay, preNeuron->neuronType);
  connectWithPreSynapse(synapse);
  preNeuron->connectWithPostSynapse(synapse);
}

void Neuron::connectWithPostSynapse(Synapse *postSynapse)
{
  postSynapses->insert(postSynapse);
}

void Neuron::connectWithPreSynapse(Synapse *preSynapse)
{
  preSynapses->insert(preSynapse);
}

void Neuron::propagate()
{
  uM = uM * tauMe; // / exp (1 / tauM);
  uS = uS * tauSe; // / exp (1 / tauS);
  uR = uR * tauRe; // / exp (1 / tauR);
  calculatePreSynapticFiring();
  if ((uM + uS + uR) >= threshold)
  {
    spikeTrain->addSpike(NOW);
    calcDelta();
    fire();
  }
//  log->log("Potential", key + " " + stringify(NOW) + " " + stringify(uM + uR + uS));
}

void Neuron::clamp(SpikeTrain* sT)
{
  cout << "Neuron::clamp: Error: can't clamp on neuron that isn't in or output." << endl;
}

void Neuron::fire()
{
  set<Synapse*>::iterator synapseIter;

  uR -= threshold;

  for(synapseIter = postSynapses->begin(); synapseIter != postSynapses->end(); synapseIter++)
    (*synapseIter)->fire();

//  log->log("Firing",  key + " " + stringify(NOW));
}

void Neuron::addIncomingSpike(WeightedFiring incomingSpike)
{
  incomingSpikes->push(incomingSpike);
}

void Neuron::errorPropagate()
{
  set<Synapse*>::iterator synapseIter;
  SpikeTrain::iterator spikeIt;
  if (spikeTrain)
  {
    // what if there's no spike...
    if (spikeTrain->empty())
    {
      // let's do something silly and make dw = + 0.000something
      spikeTrain->addSpike(40);
      spikeIt = spikeTrain->begin();
      spikeIt->setDelta(1.0);
      spikeIt->setError(3.8765);
    }
    else
    for(spikeIt = spikeTrain->begin(); spikeIt != spikeTrain->end(); spikeIt++)
      calcSpikeError(spikeIt); // actually only for output-neurons
    for(spikeIt = spikeTrain->begin(); spikeIt != spikeTrain->end(); spikeIt++)
      if (spikeIt->getError()) // efficiency
        log->log("Error", key + " " + stringify(spikeIt->getSpikeTime()) + " " + stringify(spikeIt->getError()));
    for(synapseIter = preSynapses->begin(); synapseIter != preSynapses->end(); synapseIter++)
      sendError(synapseIter); // backpropagate Error to synapse and presynaptic neuron
  }
}

void Neuron::calcDelta()
{
  double delta;
  SpikeTrain::iterator spikeIt;

  spikeIt = spikeTrain->end(); // aaaahhhh!
  spikeIt--; // this is ugly; stl is.
  
  delta = - uR / tauR - uM / tauM - uS / tauS;
  if (delta == 0) 
  {
    cout << "Neuron " << key << " ::calcDelta2(...): Warning Delta2 == 0, will cause division by zero." << endl; 
  }
  if (delta < 0.1) // ad-hoc rule to prevent large weight-changes
  {
    delta = 0.1;
  }
  
  spikeIt->setDelta(delta);
}

void Neuron::sendError(set<Synapse*>::iterator synapseIter)
{
  // this is going to get ugly... i can't see a simple way to break it up without doing calculations double.
  double s;
  double s2;
  double pSpSD;
  double pSWD;
  double refDep;
  double reftermpSWD;
  double reftermpSpSD;
  double preSpikeError;
  double weightError = 0.0;
  SpikeTrain::iterator spikeIt2;
  SpikeTrain::iterator spikeIt;
  SpikeTrain::iterator spikeIt3;
  Neuron* preNeuron = (*synapseIter)->getPreNeuron();
  SpikeTrain* preNeuronSpikeTrain = preNeuron->getSpikeTrain();
  map<SpikeTrain::iterator, double> postSpikePreSpikeDep;
  map<SpikeTrain::iterator, double> postSpikeWeightDep;

  for(spikeIt2 = preNeuronSpikeTrain->begin(); spikeIt2 != preNeuronSpikeTrain->end(); spikeIt2++)
  {
    preSpikeError = 0.0;
    postSpikePreSpikeDep.clear();
    postSpikeWeightDep.clear();
    for(spikeIt = spikeTrain->begin(); spikeIt != spikeTrain->end(); spikeIt++)
      if (spikeIt->getError()) // efficiency
      {
        s = spikeIt->getSpikeTime() - spikeIt2->getSpikeTime() - (*synapseIter)->getDelay();
        if (s>0) // check if postspike is after prespike
        {
          // calc refterm
          reftermpSWD = 0.0;
          reftermpSpSD = 0.0;
          for(spikeIt3 = spikeTrain->begin(); spikeIt3 != spikeIt; spikeIt3++)
          {
            s2 = spikeIt->getSpikeTime() - spikeIt3->getSpikeTime();
            if ((s-s2) > 0) // check if postspike spikeIt3 is after prespike
            { 
              refDep = threshold / tauR * exp(-s2 / tauR); //eta'(s2);
              reftermpSWD += refDep * postSpikeWeightDep.find(spikeIt3)->second;
              reftermpSpSD += refDep * postSpikePreSpikeDep.find(spikeIt3)->second;
            }
          }
          pSWD = (double) (- (exp(-s / tauM) - exp(-s / tauS)) + reftermpSWD) / spikeIt->getDelta();
          postSpikeWeightDep.insert(make_pair(spikeIt, pSWD));
          weightError += pSWD * spikeIt->getError();
          pSpSD = (double) ( (*synapseIter)->getWeight() * (- exp(-s / tauM) / tauM +  exp(-s / tauS) / tauS) + reftermpSpSD ) / spikeIt->getDelta(); 
          postSpikePreSpikeDep.insert(make_pair(spikeIt, pSpSD));
          preSpikeError += pSpSD * spikeIt->getError();
        }
      }
    preNeuron->signalSpikeError(spikeIt2, preSpikeError);
  }
  (*synapseIter)->signalWeightError(weightError);
}

void Neuron::signalSpikeError(SpikeTrain::iterator spikeIt, double sE)
{
  spikeIt->addError(sE);
}

void Neuron::changeWeights()
{
  set<Synapse*>::iterator synapseIter;
  for(synapseIter = preSynapses->begin(); synapseIter != preSynapses->end(); synapseIter++)
  {
    (*synapseIter)->changeWeight();
  }
}

void Neuron::reset()
{
  set<Synapse*>::iterator synapseIter;
  uM = 0.0;
  uS = 0.0;
  uR = 0.0;
  for(synapseIter = preSynapses->begin(); synapseIter != preSynapses->end(); synapseIter++)
  {
    // Hoeft denk ik niet:
    (*synapseIter)->reset();
  }
  while(!incomingSpikes->empty()) {
    incomingSpikes->pop();
  }
  spikeTrain->clear();
}

// protected functions:
void Neuron::calcSpikeError(SpikeTrain::iterator spikeIt)
{
  cout << "Neuron::calcSpikeError: Warning: no need te calculate the error of a neuron that isn't a hidden or output neuron." << endl;
}

// private functions:
void Neuron::calculatePreSynapticFiring()
{
  double weightSum = 0.0;

  while ((!incomingSpikes->empty()) && (((WeightedFiring)incomingSpikes->top()).getSpikeTime() <= NOW)) // global
  {
//    cout << getKey() << " : calculatePreSynapticFiring time = " << NOW << endl;
    weightSum += normalizeEpsilon * ((WeightedFiring) incomingSpikes->top()).getWeight();
    incomingSpikes->pop();
  }
  uM += weightSum;
  uS -= weightSum;
}

// InputNeuron:
// constructors:
InputNeuron::InputNeuron(string key, int neuronT) : Neuron(key)
{
//  current = NULL;
  spikeTrain = NULL;
  neuronType = neuronT;
}

void InputNeuron::clamp(SpikeTrain* sT)
{
  spikeTrain = sT;
  current = spikeTrain->begin();
}

void InputNeuron::propagate()
{
 // cout << "nu is mijn (" << getKey() << ")  current->getSpikeTime() = " << current->getSpikeTime() << endl;
  if (!spikeTrain)
    cout << " InputNeuron::propagate() Error: no spikeTrain clamped on input." << endl;
  else {
    if (current != spikeTrain->end()) {
      if (current->getSpikeTime() <= NOW) { // global
        current++;
        fire();
      }
    }
  }
}

void InputNeuron::reset()
{
//  current = NULL;
  spikeTrain = NULL;
}

// HiddenNeuron:
// constructors:
HiddenNeuron::HiddenNeuron(string key, int neuronT) : Neuron(key)
{
  spikeTrain = new SpikeTrain();
  neuronType = neuronT;
}

void HiddenNeuron::calcSpikeError(SpikeTrain::iterator spikeIt)
{
  // is al gedaan door postneuronen
}

// OutputNeuron:
// constructors:
OutputNeuron::OutputNeuron(string key) : Neuron(key)
{
  spikeTrain = new SpikeTrain();
  desiredSpikeTrain = NULL;
}

void OutputNeuron::clamp(SpikeTrain* dST)
{
  desiredSpikeTrain = dST;
}

void OutputNeuron::propagate()
{ 
    if (spikeTrain->empty())
          Neuron::propagate();
}

void OutputNeuron::calcSpikeError(SpikeTrain::iterator spikeIt)
{
  double sE;
  // only compare first spike -> later spikes have an error of zero:
  if(spikeIt == spikeTrain->begin()) 
    sE = spikeIt->getSpikeTime() - desiredSpikeTrain->begin()->getSpikeTime();
  else
    sE = 0.0;
  spikeIt->setError(sE);
}

void OutputNeuron::reset()
{
  Neuron::reset();
  desiredSpikeTrain = NULL;
}

// Firing:
Firing::Firing(double t)
{
  spikeTime = t;
}

double Firing::getSpikeTime()
{
  return(spikeTime);
}

// WeightedFiring:
WeightedFiring::WeightedFiring(double t, double w) :
  Firing(t)
{
  weight = w;
}

double WeightedFiring::getWeight()
{
  return(weight);
}

bool operator<(WeightedFiring a, WeightedFiring b)
{
  // the earliest spike (smallest spikeTime) should come first (bigger):
  return(a.getSpikeTime() > b.getSpikeTime());
}

FiringPlus::FiringPlus(double t) :
  Firing(t) 
{
  errorToSpike = 0.0;
  delta = 0.0;
}

FiringPlus::~FiringPlus()
{}

double FiringPlus::getError()
{
  return(errorToSpike);
}
double FiringPlus::getDelta()
{
  return(delta);
}

void FiringPlus::setError(double e)
{
  errorToSpike = e;
}

void FiringPlus::addError(double aE)
{
  errorToSpike += aE;
}

void FiringPlus::setDelta(double d)
{
  delta = d;
}

//SpikeTrain:
SpikeTrain::SpikeTrain()
{}

void SpikeTrain::printSpikeTimes()
{
  iterator firingIt;
  for(firingIt=begin(); firingIt != end(); firingIt++)
    cout << "  " << firingIt->getSpikeTime();
  cout << endl;
}

void SpikeTrain::addSpike(double spikeTime)
{
  push_back(*(new FiringPlus(spikeTime)));
}
// Synapse:
Synapse::Synapse(Neuron* prN, Neuron* poN, double initLowerBound, double initUpperBound, double lR, double d, int synapseT)
{
  preNeuron = prN;
  postNeuron = poN;
  delay = d;
  learningRate = lR;
  setLowerBound(-1000.0);
  setUpperBound(1000.0);
  if (synapseT == -1)
  {
    setUpperBound(0.0);
    if (initUpperBound > upperBound)
      initUpperBound = upperBound;
  }
  if (synapseT == 1)
  {
    setLowerBound(0.0);
    if (initLowerBound < lowerBound)
      initLowerBound = lowerBound;
  }
  initWeight(initLowerBound, initUpperBound);
  key = "Synapse-" + preNeuron->getKey() + "-" + postNeuron->getKey() + "-" + stringify(delay);
  error = 0.0;
}

Synapse::~Synapse()
{}

string Synapse::getKey()
{
  return(key);
}

double Synapse::getWeight()
{
  return(weight);
}

double Synapse::getDelay()
{
  return(delay);
}

double Synapse::getError()
{
  return(error);
}

Neuron* Synapse::getPreNeuron()
{
  return(preNeuron);
}

Neuron* Synapse::getPostNeuron()
{
  return(postNeuron);
}

void Synapse::setLowerBound(double lB)
{
  lowerBound = lB;
}

void Synapse::setUpperBound(double uB)
{
  upperBound = uB;
}

void Synapse::setLog(Logging* l)
{
  log = l;
//  log->log("Weight",  key + " " + stringify(weight)); // not the right place, but during initWeight logging isn't configured yet
}

void Synapse::printWeight()
{
  cout << key << " "<< weight << ", ";
}

void Synapse::fire()
{
  WeightedFiring wF(NOW + delay, weight);
  postNeuron->addIncomingSpike(wF);
}

void Synapse::reset()
{
//NO  error = 0.0; // for in batch-mode 
}

void Synapse::signalWeightError(double weightError)
{
//  error = weightError;
  error += weightError; // for in batch-mode 
}

void Synapse::changeWeight()
{
//  const double learningRate = 0.000002; // should be a parameter of the network
  weight += - learningRate * error;
  if (weight < lowerBound)
    weight = lowerBound;
  if (weight > upperBound)
    weight = upperBound;
//  log->log("Weight",  key + " " + stringify(weight));
  error = 0.0; // not in reset() because of batch/nonbatch difference
}

// private:
void Synapse::initWeight(double initLowerBound, double initUpperBound)
{ 
  weight = initLowerBound + (initUpperBound - initLowerBound) * ((double)rand() / RAND_MAX);
}


// Pattern:
Pattern::Pattern(string k): key(k) 
{}

Pattern::~Pattern()
{
}
   
string Pattern::getKey()
{
  return(key);
}

void Pattern::print()
{
  iterator it;
  cout << "PatternKey = " << key << endl;
  for(it = begin(); it != end(); it++)
  {
    cout << it->first << " : ";
    it->second->printSpikeTimes();
  }
}
  
/*
int main(int argc, char** argv)
{
  string BPFileName;

  if(argc == 2)
    BPFileName = argv[1];
  else {
    BPFileName = "data/2532p0.pat";
    cout << "using default BPFile: \"" << BPFileName << "\"."  << endl;
  }
  Network net(BPFileName, 2);
//  net.printLayout();
  net.printPatterns();
//  net.printWeights();
  net.trainAllPatterns();
  net.testAllPatterns();
  return(0);
}
*/
