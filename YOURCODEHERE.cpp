#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <fstream>
#include <map>
#include <math.h>
#include <fcntl.h>
#include <vector>
#include <iterator>

#include "431project.h"

using namespace std;

/*
 * Enter your PSU ID here to select the appropriate dimension scanning order.
 */
#define MY_PSU_ID 938676033

/*
 * Some global variables to track heuristic progress.
 * 
 * Feel free to create more global variables to track progress of your
 * heuristic.
 */
unsigned int getdl1size(std::string configuration);
unsigned int getil1size(std::string configuration);
unsigned int getl2size(std::string configuration);
unsigned int currentlyExploringDim = 2;
bool currentDimDone = false;
bool isDSEComplete = false;

/*
 * Given a half-baked configuration containing cache properties, generate
 * latency parameters in configuration string. You will need information about
 * how different cache paramters affect access latency.
 * 
 * Returns a string similar to "1 1 1"
 */
/*halfbacked config is a string of all parameter indexes*/
std::string generateCacheLatencyParams(string halfBackedConfig)
{

	string latencySettings;

	//
	//YOUR CODE BEGINS HERE
	//
	unsigned int dl1size = getdl1size(halfBackedConfig);
	unsigned int il1size = getil1size(halfBackedConfig);
	unsigned int ul2size = getl2size(halfBackedConfig);
	unsigned int il1lat;
	unsigned int dl1lat;
	unsigned int ul2lat;
	il1lat = log2(il1size / 1024) - 1;
	dl1lat = log2(dl1size / 1024) - 1;
	ul2lat = log2(ul2size / 1024) - 5;

	int il1ass = extractConfigPararm(halfBackedConfig, 6);
	int dl1ass = extractConfigPararm(halfBackedConfig, 4);
	int ul2ass = extractConfigPararm(halfBackedConfig, 9);
	il1lat = il1lat + il1ass;
	dl1lat = dl1lat + dl1ass;
	ul2lat = ul2lat + ul2ass;

	latencySettings = "1 1 1";
	latencySettings[0] = '0' + dl1lat;
	latencySettings[2] = '0' + il1lat;
	latencySettings[4] = '0' + ul2lat;
	//
	//YOUR CODE ENDS HERE
	//

	return latencySettings;
}

/*
 * Returns 1 if configuration is valid, else 0
 */
int validateConfiguration(std::string configuration)
{

	// FIXME - YOUR CODE HERE
	bool valid = true;

	//second parameter gives you that index
	int width = extractConfigPararm(configuration, 0);
	int il1blindex = extractConfigPararm(configuration, 2);
	int ul2blindex = extractConfigPararm(configuration, 8);
	int ul2blsize = pow(2, (ul2blindex + 4));
	int il1blsize = pow(2, (il1blindex + 3));

	unsigned int dl1size = getdl1size(configuration) / 1024; //sizes in KB
	unsigned int il1size = getil1size(configuration) / 1024;
	unsigned int ul2size = getl2size(configuration) / 1024;
	// The below is a necessary, but insufficient condition for validating a
	// configuration.
	if (isNumDimConfiguration(configuration) != 1)
	{
		valid = false;
	}
	if (width != il1blindex) //constraint 1
	{
		valid = false;
	}
	if (ul2blsize < (2 * (il1blsize)))
	{ //constraint2
		valid = false;
	}
	if (ul2size < (2 * (il1size + dl1size)))
	{ //constraint 2
		valid = false;
	}
	if ((il1size < 2) || (dl1size < 2) || (il1size > 64) || (dl1size > 64))
	{ //constraint 3
		valid = false;
	}
	if ((ul2size < 32) || (ul2size > 1024))
	{ //constraint 4
		valid = false;
	}

	return valid;
}

/*
 * Given the current best known configuration, the current configuration,
 * and the globally visible map of all previously investigated configurations,
 * suggest a previously unexplored design point. You will only be allowed to
 * investigate 1000 design points in a particular run, so choose wisely.
 *
 * In the current implementation, we start from the leftmost dimension and
 * explore all possible options for this dimension and then go to the next
 * dimension until the rightmost dimension.
 */
//currentconfig: from proposeconfig
//best execonfig: performance
//best EDP config: best energy
//last 2 parameters: set to 1 for energy/performance, 0 the other

std::string generateNextConfigurationProposal(std::string currentconfiguration,
											  std::string bestEXECconfiguration, std::string bestEDPconfiguration,
											  int optimizeforEXEC, int optimizeforEDP)
{

	//
	// Some interesting variables in 431project.h include:
	//
	// 1. GLOB_dimensioncardinality
	// 2. GLOB_baseline
	// 3. NUM_DIMS
	// 4. NUM_DIMS_DEPENDENT
	// 5. GLOB_seen_configurations

	std::string nextconfiguration = currentconfiguration;
	// Check if proposed configuration has been seen before.
	int nextValue = -1;
	int dimCounter = 0;
	while (!validateConfiguration(nextconfiguration) || GLOB_seen_configurations[nextconfiguration])
	{

		// Check if DSE has been completed before and return current
		// configuration.
		//don't change
		if (isDSEComplete)
		{
			return currentconfiguration;
		}

		std::stringstream ss;

		string bestConfig;
		if (optimizeforEXEC == 1)
			bestConfig = bestEXECconfiguration;

		if (optimizeforEDP == 1)
			bestConfig = bestEDPconfiguration;

		// Fill in the dimensions already-scanned with the already-selected best
		// value.
		//right now, ss is every value up to the one we are checking
		for (int dim = 0; dim < currentlyExploringDim; ++dim)
		{
			ss << extractConfigPararm(bestConfig, dim) << " ";
		}

		// Handling for currently exploring dimension. This is a very dumb
		// implementation.
		nextValue++;
		//set the dimension we are checking to the next index for that dimension
		if (nextValue >= GLOB_dimensioncardinality[currentlyExploringDim])
		{ //if there are no more for that index, set it back to what it was and move to next dimension
			nextValue = GLOB_dimensioncardinality[currentlyExploringDim] - 1;
			currentDimDone = true;
			dimCounter++;
		}
		// add the value we are checking if valid

		ss << nextValue << " ";

		// Fill in remaining independent params with 0.
		//1. don't need to change the for loop right?
		for (int dim = (currentlyExploringDim + 1);
			 dim < (NUM_DIMS - NUM_DIMS_DEPENDENT); ++dim)
		{													   //instead of zero need to fill in with the best config values
															   //is this right?
			ss << extractConfigPararm(bestConfig, dim) << " "; //1
															   //dont keep adding 0 here... add best performaning index
		}

		//
		// Last NUM_DIMS_DEPENDENT3 configuration parameters are not independent.
		// They depend on one or more parameters already set. Determine the
		// remaining parameters based on already decided independent ones.
		//
		//this has first 15, caluclate last 3 based on cache values
		string configSoFar = ss.str();

		// Populate this object using corresponding parameters from config.
		ss << generateCacheLatencyParams(configSoFar);

		// Configuration is ready now.
		nextconfiguration = ss.str();

		// Make sure we start exploring next dimension in next iteration.
		if (currentDimDone)
		{
			nextValue = -1;
			if (currentlyExploringDim == 10)
			{
				currentlyExploringDim = 0;
				currentDimDone = false;
			}
			else if (currentlyExploringDim == 1)
			{
				currentlyExploringDim = 11;
				currentDimDone = false;
			}
			else if (currentlyExploringDim == 11)
			{
				currentlyExploringDim = 12;
				currentDimDone = false;
			}
			else if (currentlyExploringDim == 14)
			{
				currentlyExploringDim = 2;
				currentDimDone = false;
			}
			else
			{
				currentlyExploringDim++;
				currentDimDone = false;
			}
		}

		// Signal that DSE is complete after this configuration.
		if (dimCounter == 15) //this is 15

			isDSEComplete = true;
		// Keep the following code in this function as-is.
		if (!validateConfiguration(nextconfiguration))
		{
			cerr << "Exiting with error; Configuration Proposal invalid: "
				 << nextconfiguration << endl;
			continue;
		}
	}
	return nextconfiguration;
}
