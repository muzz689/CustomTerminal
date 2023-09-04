#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <dirent.h> // Include for directory handling
#include <ctype.h>	// Include for isspace()

#define PATH_MAX_LENGTH 1024				  // Maximum length for path strings
#define MAX_PATH_DIRECTORIES 10				  // Maximum length for path strings
char *path[MAX_PATH_DIRECTORIES] = {"/bin/"}; // Global variable for path initialize with the default path
int path_count = 1;

// One and only Error Handle
void handleError()
{
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
	return;
}

/* Handles Concating
 * Takes in output array ,  and input given and concats it with executable paths
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

// Function to skip leading whitespace in a string
char *skipLeadingWhitespace(char *str)
{
	while (*str != '\0' && isspace((unsigned char)*str))
	{
		str++;
	}
	return str;
}

// Function to find the position of the first redirection operator or NULL if not found
char *findRedirectionOperator(char *str, bool *redirectFlag, bool *multipleRedirect)
{
	*multipleRedirect = false;	// Initialize the flag to false
	*redirectFlag = false;		// Initialize the flag to false
	char *firstRedirect = NULL; // Initialize the pointer to the first redirection operator

	while (*str != '\0')
	{
		if (*str == '>')
		{
			if (firstRedirect == NULL)
			{
				// Set the pointer to the first redirection operator
				firstRedirect = str;
				*redirectFlag = true;
			}
			else
			{
				// Multiple redirection operators found
				*multipleRedirect = true;
			}
		}

		str++;
	}

	return firstRedirect; // Return the pointer to the first redirection operator
}

char *CheckRedirection(char *input, bool *redirectFlag, bool *multiredirectFlag, char **outputFile)
{

	// Find the position of the first redirection operator
	char *redirect = findRedirectionOperator(input, redirectFlag, multiredirectFlag); // >outputfile part

	// Initialize variables to store the command and output file
	char *command = NULL;
	// char *outputFile = NULL;

	if (redirect != NULL)
	{
		// If redirection operator found, split the input into command and output file
		*redirect = '\0'; // Null-terminate the command part

		command = input;
		*outputFile = skipLeadingWhitespace(redirect + 1); // outputfile part

		// TO ENSURE WE DONT HAVE MULTIPLE OUTPUT FILES
		if (strstr(*outputFile, " ") != NULL)
		{
			*multiredirectFlag = true;
		}

		// if(strstr)

		// Remove trailing whitespace from the command and output file
		char *end = command + strlen(command) - 1;
		while (end > command && isspace((unsigned char)*end))
		{
			*end = '\0';
			end--;
		}
		end = *outputFile + strlen(*outputFile) - 1;
		while (end > *outputFile && isspace((unsigned char)*end))
		{
			*end = '\0';
			end--;
		}
	}
	else
	{
		// If no redirection operator is found, treat the entire input as the command
		command = input;
	}

	// // Count the number of space characters in the outputFile string
	// int spaceCount = 0;
	// char *outputFilePtr = *outputFile;
	// while (*outputFilePtr != '\0') {
	//     if (*outputFilePtr == ' ') {
	//         spaceCount++;
	//     }
	//     outputFilePtr++;
	// }

	// // If there are more than one space characters, it means there are multiple output file specifications
	// if (spaceCount > 1) {
	//     *multiredirectFlag = true;
	// }

	return command;
}

/* Handles non-basic commands
 * Takes in fullpath , input
 */
void handleCommandls(char full_path[], char *input)
{
	bool redirectFlag = false;
	bool multiredirectFlag = false;
	// char outputFile[PATH_MAX_LENGTH] = "";
	char *outputFile = NULL;

	// Trim leading and trailing whitespace
	input = skipLeadingWhitespace(input);

	input = CheckRedirection(input, &redirectFlag, &multiredirectFlag, &outputFile);
	/* Splits the whole input by whitespace into args array
	 *	Example : (cd tester = args={"cd","tester",NULL})
	 */
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

	/* Check for   redirection and outputFile
	 * Loops through args array
	 * if it finds 1 > contained within an arg, then finds output file after it provided its not null.
	 * if its still in loop and already found redirection meaning either multiple > or multiple outfiles
	 * Sets the multiredirectFlag = True.
	 */

	// for (int i = 0; args[i] != NULL; i++)
	// {

	// 	if (strcmp(args[i], ">") == 0 && !redirectFlag )
	// 	{
	// 		redirectFlag = true;
	// 		args[i] = NULL;
	// 		if (args[i + 1] != NULL)
	// 		{
	// 			strcpy(outputFile, args[i + 1]);

	// 			i++;
	// 		}
	// 	}
	// 	else if (redirectFlag)
	// 	{
	// 		multiredirectFlag = true;

	// 	}

	// }

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
			// If we Redirecting
			if (redirectFlag)

			{
				// printf("awe %d \n", multiredirectFlag);

				// If 1 outputfile and no multi > or multi files
				if (outputFile != NULL && !multiredirectFlag)
				{

					int outputFd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
					if (outputFd == -1)
					{
						handleError();
						exit(EXIT_FAILURE);
					}
					dup2(outputFd, STDOUT_FILENO);
					dup2(outputFd, STDERR_FILENO); // Redirect stderr to the same file
					close(outputFd);
					execv(executable_path, args);
				}
				//  No file or Multi > or Multi file
				else

				{
					handleError();
				}
			}
			// Normal execution
			else
			{
				execv(executable_path, args);
			}
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

		path[i] = strdup(newPaths[i]);
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

	// Check if there are additional arguments (i.e., more than one file)
	else if (MainArgc > 2)
	{
		handleError();
		exit(EXIT_FAILURE);
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
					printf("%s", path[i]);
				}
				printf("hsb");
			}

			// Non Basic Commands
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