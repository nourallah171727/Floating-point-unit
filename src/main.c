#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdbool.h>

#include "../include/MainHelpers.h"

int main(int argc, char *argv[])
{

	if (argc > 13)
	{
		printf("Too many arguments\n");
		exit(1);
	}

	// initialising the main program's variables:
	struct Request *requests;

	uint32_t cycles = UINT32_MAX;
	uint32_t sizeExponent;
	uint32_t sizeMantissa;
	uint32_t roundMode = 0; // defaukt rounding
	uint32_t numRequests;

	const char *tracefile;

	// Getting Input with getopt_long:
	int opt;
	int option_index = 0;

	// listing all the options
	//(Note : we are considering only flags in form --A and not -A) as described in the Project's description
	static struct option long_options[] = {
		{"help", no_argument, 0, 0},
		{"tf", required_argument, 0, 0},
		{"cycles", required_argument, 0, 0},
		{"size-exponent", required_argument, 0, 0},
		{"size-mantissa", required_argument, 0, 0},
		{"round-mode", required_argument, 0, 0},
		{0, 0, 0, 0}};
	// Track what the user actually set
	bool user_set_exponent = false;
	bool user_set_mantissa = false;

	while ((opt = getopt_long(argc, argv, "", long_options, &option_index)) != -1)
	{
		if (opt == 0)
		{
			const char *optname = long_options[option_index].name;

			if (strcmp(optname, "help") == 0)
			{
				printHelp();
				// should be exit 0 not 1
				exit(0);
			}
			else if (strcmp(optname, "tf") == 0)
			{
				tracefile = optarg; // check is the path is valid
			}
			else if (strcmp(optname, "cycles") == 0)
			{
				cycles = convert_str_32(optarg);
			}
			else if (strcmp(optname, "size-exponent") == 0)
			{
				sizeExponent = convert_str_8(optarg);
				user_set_exponent = true;
			}
			else if (strcmp(optname, "size-mantissa") == 0)
			{
				sizeMantissa = convert_str_8(optarg);
				user_set_mantissa = true;
			}
			else if (strcmp(optname, "round-mode") == 0)
			{
				roundMode = convert_str_8(optarg);
			}
		}
		else if (opt == '?')
		{
			// Unknown/invalid option
			fprintf(stderr, "Unknown or invalid option. Use --help for usage.\n");
			exit(1);
		}
	}
	// Now resolve logic after parsing:
	if (user_set_exponent && !user_set_mantissa)
	{
		if (sizeExponent == 0 || sizeExponent >= 31)
		{
			fprintf(stderr, "Exponent must be between 1 and 30.\n");
			exit(1);
		}
		sizeMantissa = 31 - sizeExponent;
	}
	else if (!user_set_exponent && user_set_mantissa)
	{
		if (sizeMantissa == 0 || sizeMantissa >= 31)
		{
			fprintf(stderr, "Mantissa must be between 1 and 30.\n");
			exit(1);
		}
		sizeExponent = 31 - sizeMantissa;
	}
	else if (!user_set_exponent && !user_set_mantissa)
	{
		// Both defaults: sizeExponent=8, sizeMantissa=23
		sizeExponent = 8;
		sizeMantissa = 23;
	}
	else
	{
		// Both are set: check validity
		if (sizeExponent + sizeMantissa != 31)
		{
			fprintf(stderr, "Size Mantissa + Size Exponent should be = 31\n");
			exit(1);
		}
		if (sizeExponent == 0 || sizeMantissa == 0)
		{
			fprintf(stderr, "Exponent and mantissa must not be 0.\n");
			exit(1);
		}
	}
	printf("exponent bits are %u\n", sizeExponent);
	printf("exponent bits are %u\n", sizeMantissa);

	// Get .csv file:
	if (optind >= argc)
	{
		printf("Missing .csv file argument\n");
		exit(1);
	}
	if (optind + 1 < argc)
	{
		fprintf(stderr, "Error: Unexpected extra arguments after input file: ");
		for (int i = optind + 1; i < argc; ++i)
		{
			fprintf(stderr, "%s ", argv[i]);
		}
		fprintf(stderr, "\n");
		exit(1);
	}

	const char *filename = argv[optind];
	// verify if it's a csv file
	if (strlen(filename) < 5 || strcmp(filename + strlen(filename) - 4, ".csv") != 0)
	{
		printf("The .csv file isn't valid\n");
		exit(1);
	}

	// Debug
	// printf("inputs: %d %d %d %d %s\n", cycles, sizeExponent, sizeMantissa, roundMode, tracefile);

	// TODO: extract Requests from .csv file + num Requests
	requests = load_csv_requests(filename, sizeExponent, sizeMantissa, roundMode, &numRequests, cycles);

	// TODO: call run_simulation with all the inputs..
	struct Result r = run_simulation(
		cycles, tracefile, sizeExponent, sizeMantissa,
		roundMode, numRequests, requests);

	printf("Simulation results:\n");

	for (int i = 0; i < numRequests; i++)
	{
		printf("0x%08" PRIx32 " ", requests[i].ro);
	}

	printf("\nCycles: %u\nSigns: %u\nOverflows: %u\nUnderflows: %u\nInexact: %u\nNaNs: %u\n",
		   r.cycles, r.signs, r.overflows, r.underflows, r.inexacts, r.nans);

	// free
	free(requests);

	return 0;
}