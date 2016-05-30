// spikeprop.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "spikeProp2.cc"

int main(int argc, char** argv)
{
	Network* net;
	string BPFileName;
	int i;

	if (argc == 2)
		BPFileName = argv[1];
	else {
		BPFileName = "data/XorBohteConAan.pat";
		cout << "using default BPFile: \"" << BPFileName << "\"." << endl;
	}

	// Network(
	//         string   Pattern file name,
	//         string   Extra log key,
	//         bool     batchlearning-switch,
	//         double   time-range of delays,
	//         int      nr of delayed synapses,
	//         double   init lower weight value,
	//         double   init upper weight value,
	//         double   learning-rate,
	//         double   stopping-criterium sum squared error
	//        )
	net = new Network(BPFileName, "", 0, 16, 16, -1.0, 2.0, 0.01, 1.0);
	//  net->printLayout();
	//  net->printPatterns();
	//  net->printWeights();
	net->trainAllPatterns(100000);
	net->testAllPatterns();

	return(0);
}

