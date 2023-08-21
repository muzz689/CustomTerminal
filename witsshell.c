#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <dirent.h> // Include for directory handling

#define PATH_MAX_LENGTH 1024 // Maximum length for path strings

//One and only Error Handle
void handleError()
{
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
	return;
}

/* Handles Concating
* Takes in output array , path and input given and concats it
*/  
void FormatPath(char full_path[], char *path, char *input)
{

	snprintf(full_path, PATH_MAX_LENGTH, "%s%s", path, input);
	return;
}
/* Handles non-basic commands 
* Takes in fullpath , input , path
*/
void handleCommandls(char full_path[], char *input, char *path)
{
	// Split the whole input by whitespace
	char *command = strtok(input, " ");
	char *args[100]; 					// Assuming a maximum of 100 arguments
	int argIndex = 0;
	while (command != NULL)				//Continue spliting
	{
		args[argIndex] = command;
		argIndex++;
		command = strtok(NULL, " ");
	}
	args[argIndex] = NULL; 			  // Last element needs to be null for execv


	// To get the exec path with just  /bin/{command}
	char executable_path[PATH_MAX_LENGTH];
	FormatPath(executable_path, path, args[0]); 

		// Check if the file exists and is executable
		if (access(executable_path, X_OK) == 0)
	{

		// Create a new process to run the command
		pid_t child_pid = fork();
		if (child_pid == 0)
		{

			execv(executable_path, args);
		}
		else if (child_pid > 0)
		{
			// Parent process
			wait(NULL); // Wait for the child process to complete
		}
		else
		{
			handleError();
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		handleError();
		exit(EXIT_FAILURE);
	}

	return;
}

int main(int MainArgc, char *MainArgv[])
{

	char *input = NULL; // Take in input
	size_t input_size = 0;
	char *path = "/bin/"; // Set the initial shell path to "/bin/"

	// Batch
	if (MainArgc == 2)
	{
		// Open batch file
		FILE *batch_file = fopen(MainArgv[1], "r");
		if (batch_file == NULL)
		{
			// Exit 1
			exit(EXIT_FAILURE);
		}

		while (getline(&input, &input_size, batch_file) != -1)
		{
			// Remove the newline character
			input[strcspn(input, "\n")] = '\0';

			// Exit 0
			if (strcmp(input, "exit") == 0)
			{
				exit(EXIT_SUCCESS);
			}

			// Non basic commmand
			else
			{
				char full_path[PATH_MAX_LENGTH];
				FormatPath(full_path, path, input);
				handleCommandls(full_path, input, path);
				free(input);
			}
		}
		free(input);
		return (0);
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

			// Exit 0 command
			if (strcmp(input, "exit") == 0)
			{
				free(input);
				exit(EXIT_SUCCESS);
			}

			// Non Basic Functions
			else
			{

				char full_path[PATH_MAX_LENGTH];
				FormatPath(full_path, path, input);
				handleCommandls(full_path, input, path);
				// free(input);
			}
		}

		free(input);

		return (0);
	}
}