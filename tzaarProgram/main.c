/*
 * The module main parses command line arguments, loads the position and
 * starts the search.
 * 
 * Author: Pavel Veselý
 * License: GPL v3, see license.txt
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <getopt.h>

#include "main.h"

void printHelp() {
	printf("Searches for the best moves in a position in Tzaar: \n");
	printf("\t-a AI --ai\t AI number (1-9, 20-25, 40-42)\n");
	printf("\t-b FILE --bestmove=FILE\t Search for the best moves in a position stored in FILE. This is required option.\n");
	printf("\t-e FILE --execute=FILE\t Execute the the best moves and then save the position to FILE.\n");
	printf("\t-t SECONDS --timelimit=SECONDS\t Set time limit of the search to SECONDS (default is %d).\n", AI_TIME_LIMIT);
}

i32 main(i32 argc, char *argv[])
{
	i32 ai = -1;
	i32 time = AI_TIME_LIMIT; // in seconds
	char *executeFile = null;
	char *fileWithPosition = null;
	i32 c, option_index;
	while ((c = getopt_long(argc, argv, options, long_options, &option_index)) >= 0) {
		switch (c) {
		case 'a':
			sscanf(optarg, "%d", &ai);
			DPRINT2("Argument ai: %d", ai);
			break;
		case 'b':
			fileWithPosition = (char *) malloc(sizeof(char) * (strlen(optarg) + 5));
			strcpy(fileWithPosition, optarg);
			break;
		case 'e':
			executeFile = (char *) malloc(sizeof(char) * (strlen(optarg) + 5));
			strcpy(executeFile, optarg);
			DPRINT2("Position after executing best moves will be saved in '%s'", executeFile);
			break;
		case 'h':
			printHelp();
			return 0;
		case 't':
			sscanf(optarg, "%d", &time);
			DPRINT2("argument time limit: %d", time);
			break;
		default:
			printf("Not known argument '%c'. Type tzaar -h for help.\n", c);
			break;
		}
	}
	if (fileWithPosition == null) {
		printf("File with a position was not specified. Printing usage:\n");
		printHelp();
		return 0;
	}
	i32 err;
	if ((err = ProcessPosition(ai, time, fileWithPosition, fileWithPosition, executeFile)) != OK) {
		printf("Processing the position failed with error %d.\n", err);
	}
	return err;
}

i32 ProcessPosition(i32 ai, i32 time, const char *fileWithPosition, const char *fileBestMoves, const char *fileEorExecutedPos)
{
	if (fileWithPosition == null)
		return FILE_WITH_POSITION_NULL;
	if (fileBestMoves == null)
		return FILE_FOR_BEST_MOVES_NULL;
	i32 err;
	if ((err = LoadPosition(fileWithPosition)) != OK) {
		printf("Cannot load file '%s' to get best moves (err %d). Usage: ./tzaar -b [FILE]\n", fileWithPosition,
		       err);
		return CANNOT_LOAD_POSITION;
	}
	DPRINT2("selected ai: %d", ai);
	if (moveNumber == 2) {	// should not happen
		printf("Cannot get best moves for the second move of player's turn (ai %d).", ai);
		return BAD_STONE;
	}
	Move *m1, *m2;
	if (GetBestMove(ai, time, &m1, &m2, true)) {
		if ((err = SaveBestMoves(fileBestMoves, m1, m2)) != OK) {
			printf("Cannot save best moves in file '%s' (err %d)\n", fileBestMoves, err);
			return CANNOT_SAVE_POSITION;
		}
		if (fileEorExecutedPos != null) {
			value = 0;	//because of tests
			ExecuteMove(m1);
			if (m2 != null)
				ExecuteMove(m2);
			DPRINT2("Saving the executed position to '%s'", fileEorExecutedPos);	// nic nerekne o case :)
			SavePosition(fileEorExecutedPos);
		}
		return OK;
	} else {
		printf("Cannot get best moves (ai %d).", ai);
		return GET_BEST_MOVE_ERROR;
	}
}
