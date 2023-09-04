#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <dirent.h> // Include for directory handling

#define PATH_MAX_LENGTH 1024	// Maximum length for path strings
#define MAX_PATH_DIRECTORIES 10 // Maximum length for path strings
// Global variable for path initialize with the default path
char *path[MAX_PATH_DIRECTORIES] = {"/bin/"};
int path_count = 1;

// One and only Error Handle
void handleError()
{
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
	return;
}

/* Handles Concating
 * Takes in output array , path and input given and concats it
 */
void FormatPath(char full_path[], char *input)
{
	for (int i = 0; i < path_count; i++)
	{
		snprintf(full_path, PATH_MAX_LENGTH, "%s%s", path[i], input);
		if (access(full_path, X_OK) == 0)
		{
			return; // Found the executable in one of the directories
		}
	}
	full_path[0] = '\0'; 
	// If not found in any directory, full_path will contain the last directory in the path
}

/* Handles non-basic commands
 * Takes in fullpath , input , path
 */
void handleCommandls(char full_path[], char *input)
{
	// Split the whole input by whitespace
	char *command = strtok(input, " ");
	char *args[100]; // Assuming a maximum of 100 arguments
	int argIndex = 0;
	while (command != NULL) // Continue spliting
	{
		args[argIndex] = command;
		argIndex++;
		command = strtok(NULL, " ");
	}
	args[argIndex] = NULL; // Last element needs to be null for execv

	// To get the exec path with just  /bin/{command}
	char executable_path[PATH_MAX_LENGTH];
	FormatPath(executable_path, args[0]);
	
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
		// exit(EXIT_FAILURE);
	}

	return;
}

/* Handles Updating of Paths
 * Takes in old path array , new Path array , new path array count
 */

void updatePath(char **path, char *newPaths[], int newPathsCount)
{
	// Fistly  Clear the old paths
	for (int i = 0; i < path_count; i++)
	{
		path[i] = NULL;
	}

	// Copy the new paths into the array
	for (int i = 0; i < newPathsCount; i++)
	{	
		// if(strncmp(newPaths[i],"/",1)!=0){
			// newPaths[i] =  strcat("/",newPaths[i]);
			
		// }
		
		path[i] = strdup(newPaths[i]);
		// printf("%s",path[i]);
	}

	path_count = newPathsCount;

}

int main(int MainArgc, char *MainArgv[])
{

	char *input = NULL; // Take in input
	size_t input_size = 0;
	// char *path = "/bin/"; // Set the initial shell path to "/bin/"

	char *dir = NULL;

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
			// CD command
			else if (strncmp(input, "cd", 2) == 0) // First 2 is cd
			{
				dir = input + 3;
				if (chdir(dir) != 0) // Fail
				{
					handleError();
				}
			}
			// Path Command
			else if (strncmp(input, "path", 4) == 0)
			{
				int newPathsCount = 0;
				char *newPaths[MAX_PATH_DIRECTORIES];

				// Tokenize the paths and add to array
				char *token = strtok(input + 5, " ");

				while (token != NULL && newPathsCount < MAX_PATH_DIRECTORIES)
				{
					newPaths[newPathsCount++] = token;
					token = strtok(NULL, " ");
				}

				updatePath(path, newPaths, newPathsCount);
				
			}

			// Non basic commmand
			else
			{
				char full_path[PATH_MAX_LENGTH];
				FormatPath(full_path, input);
				handleCommandls(full_path, input);
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
			// CD command
			else if (strncmp(input, "cd", 2) == 0) // First 2 is cd
			{
				dir = input + 3;
				if (chdir(dir) != 0) // Fail
				{
					handleError();
				}
			}
			// Path Command
			else if (strncmp(input, "path", 4) == 0)
			{

				int newPathsCount = 0;
				char *newPaths[MAX_PATH_DIRECTORIES];

				// Tokenize the paths and add to array
				char *token = strtok(input + 5, " ");

				while (token != NULL && newPathsCount < MAX_PATH_DIRECTORIES)
				{
					newPaths[newPathsCount++] = token;
					token = strtok(NULL, " ");
				}

				updatePath(path, newPaths, newPathsCount);
			}
			else if (strncmp(input, "hello", 5) == 0)
			{
				for (int i = 0; i < path_count; i++)
				{
					printf("%s",path[i]);
				}
				printf("hsb");
			}

			// Non Basic Functions
			else
			{

				char full_path[PATH_MAX_LENGTH];
				FormatPath(full_path, input);
				handleCommandls(full_path, input);
				// free(input);
			}
		}

		free(input);

		return (0);
	}
}