#include <stdio.h> 
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include "../include/structs.h"
#include "MainHelpers.c"

int main(int argc, char* argv[]){

	if(argc>13){
		printf("Too many arguments\n");
		exit(1);
	}

	//initialising the main program's variables:
	struct Result r;
	struct Request* requests;

	uint32_t cycles, sizeExponent, sizeMantissa, roundMode;
	uint32_t numRequests;

	const char* tracefile;

	//Getting Input with getopt_long:
	int opt;
	int option_index = 0;

	//listing all the options
	//(Note : we are considering only flags in form --A and not -A) as described in the Project's description
	static struct option long_options[] ={
		{"help",no_argument,0,0},
		{"tf",required_argument,0,0},
		{"cycles",required_argument,0,0},
		{"size-exponent",required_argument,0,0},
		{"size-mantissa",required_argument,0,0},
		{"round-mode",required_argument,0,0},
		{0,0,0,0}
	};


	while((opt = getopt_long(argc, argv, "",long_options, &option_index))!=-1){
		if(opt==0){
			const char* optname = long_options[option_index].name;

			if(strcmp(optname, "help")==0){
				printHelp();
				exit(1);

			}else if(strcmp(optname, "tf")==0){
				tracefile= optarg;

			}else if(strcmp(optname, "cycles")==0){
				cycles = convert_str_32(optarg);
			}else if(strcmp(optname, "size-exponent")==0){
				sizeExponent = convert_str_8(optarg);
			}else if(strcmp(optname, "size-mantissa")==0){
				sizeMantissa = convert_str_8(optarg);
			}else if(strcmp(optname, "round-mode")==0){
				roundMode = convert_str_8(optarg);
			}
		}
	}


	//Get .csv file:
	if(optind>=argc){
		printf("Missing .csv file argument\n");
		exit(1);
	}

	const char* filename = argv[optind];
	//verify if it's a csv file
	if(strlen(filename)< 5 || strcmp(filename+strlen(filename)-4,".csv")!=0){
		printf("The .csv file isn't valid\n");
		exit(1);
	}

	//Debug
	printf("inputs: %d %d %d %d %s\n", cycles, sizeExponent, sizeMantissa, roundMode, tracefile);

	//TODO: extract Requests from .csv file + num Requests
	requests = load_csv_requests(filename, &numRequests);

	//TODO: call run_simulation with all the inputs..
	struct Result r = run_simulation(
		cycles, tracefile, sizeExponent, sizeMantissa,
		roundMode, numRequests, requests );
	printf("Simulation results:\n");
    printf("Cycles: %u\nSigns: %u\nOverflows: %u\nUnderflows: %u\nInexact: %u\nNaNs: %u\n",
       r.cycles, r.signs, r.overflows, r.underflows, r.inexactes, r.nans);


    return 0;
}