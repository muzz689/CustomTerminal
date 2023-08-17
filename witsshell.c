#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>



int main(int MainArgc, char *MainArgv[])
{

	char *input = NULL;
	size_t input_size = 0;

	// Batch
	if (MainArgc == 2)
	{	
		//Open batch file
		FILE *batch_file = fopen(MainArgv[1], "r");
		if (batch_file == NULL)
		{
			//Exit 1
			exit(EXIT_FAILURE);
		}

		while (getline(&input, &input_size, batch_file) != -1)
		{
			// Remove the newline character
			input[strcspn(input, "\n")] = '\0';

			//Exit 0
			if (strcmp(input, "exit") == 0)
			{
				exit(EXIT_SUCCESS);
			}
			
		}

	}
	// Interactive
	else
	{
		while (1)
		{
			printf("witshell > ");
			ssize_t read_bytes = getline(&input, &input_size, stdin);

			// Check if there was an error when reading input through getline() / it handles ctrl + D
			if (read_bytes == -1)
			{
				// Exit 1
				exit(EXIT_FAILURE);
			}

			// Remove the newline character and assinging it null value
			input[strcspn(input, "\n")] = '\0'; // this means null

			//Exit command
			if (strcmp(input, "exit") == 0)
			{
				
				exit(EXIT_SUCCESS);
			}

			
			
			
			
		}
	}

	free(input);

	return (0);
}
