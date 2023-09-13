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

/* Handles outputing of all Errors
 */
void handleError()
{
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
	return;
}

/* Handles Concatanation
 *Concat input with path and place it into fullpath
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
}

/* Handles removing of leading whitespace in a string
 */
char *skipLeadingWhitespace(char *str)
{
	while (*str != '\0' && isspace((unsigned char)*str))
	{
		str++;
	}
	return str;
}

/* Handles removing leading+ trailing of whitespace in a string
 */
char *skipWhitespace(char *str)
{
	char  *end;

	// Skip leading whitespace
	while (isspace((unsigned char)*str))
	{
		str++;
	}

	// Check for an empty string
	if (*str == '\0')
	{
		return str;
	}

	// Find the end of the string
	end = str + strlen(str) - 1;

	// Skip trailing whitespace by moving the end pointer backwards
	while (end > str && isspace((unsigned char)*end))
	{
		end--;
	}

	// Null-terminate the string after the trailing whitespace
	*(end + 1) = '\0';

	return str;
}

/* Handles finding the position of >
 *Find > position
 *Check for multiple >
 *Returns Position or NUll if not found
 */
char *findRedirectionOperator(char *str, bool *redirectFlag, bool *multipleRedirect)
{
	*multipleRedirect = false;	
	*redirectFlag = false;		
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

/*Handles processing of  Redirection
 *Finds redirection Operator (>)
 *Checks for multiple output files
 *Removes Trailing Whitespace  from command and outputFiles
 *Returns the command
 */
char *CheckRedirection(char *input, bool *redirectFlag, bool *multiredirectFlag, char **outputFile)
{

	// Find the position of the first redirection operator
	char *redirect = findRedirectionOperator(input, redirectFlag, multiredirectFlag); // >outputfile part
	
	// Initialize variables to store the command and output file
	char *command = NULL;
// If redirection operator found, split the input into command and output file
	if (redirect != NULL)
	{
		
		*redirect = '\0'; // Null-terminate the command part

		command = input;
		*outputFile = skipLeadingWhitespace(redirect + 1); // outputfile part

		// Check for  multiple output specification
		char *cmd = strtok(*outputFile, " ");
		int argIndex = 0;
		while (cmd != NULL) // Continue spliting
		{
			argIndex++;
			if(argIndex > 1){
				*multiredirectFlag = true;
			}
			cmd = strtok(NULL, " ");
		}

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

	return command;
}

/*Executes command
 * Execute redirection or normal execution depending on redirect Flag using execv
 */
void CommandExecute(char executable_path[], char *args[], bool redirectFlag, bool multiredirectFlag, char *outputFile)
{
	
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
				
				// If 1 outputfile and no multi > or no multi files
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
				//  No file specified or Multiple > or Multi file specified
				else

				{
					handleError();
				}
			}
			// Normal execution of non basic commands.
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
		//Anything besides 0 and greater than 0 is error + exit.
		else
		{
			handleError();
			exit(EXIT_FAILURE);
		}
	}
	//File not executable 
	else
	{
		handleError();
	}
	return;
}
/*Executes command in parallel
 * Execute redirection or normal execution depending on redirect Flag using execv
 *Similar to the normal Command Execute function except parent process doesnt wait.
 */
void CommandExecuteParallel(char executable_path[], char *args[], bool redirectFlag, bool multiredirectFlag, char *outputFile)
{
	
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
			exit(EXIT_SUCCESS);
		}

		
	}

	else
	{
		handleError();
	}
	return;
}

/* Handles non-basic commands
 * Takes in  input , runparallel flag
 * Checks for redirection
 * Split  input by " "
 * Execute the command based on runParallel
 */
void handleCommandls( char *input,bool runParallelFlag)
{
	// Variable for redirection
	bool redirectFlag = false;
	bool multiredirectFlag = false;
	char *outputFile = NULL;

	// Trim leading and trailing whitespace
	input = skipLeadingWhitespace(input);

	// Check Redirection
	input = CheckRedirection(input, &redirectFlag, &multiredirectFlag, &outputFile);

	/* Splits the whole input by whitespace into args array
	 *	Example : (ls tester = args={"ls","tester",NULL})
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

	// To get the exec path eg : /bin/ls
	char executable_path[PATH_MAX_LENGTH];

	FormatPath(executable_path, args[0]);


	// Execute command
	
	if(runParallelFlag){
		CommandExecuteParallel(executable_path, args, redirectFlag, multiredirectFlag, outputFile);
	}
	else{
		CommandExecute(executable_path, args, redirectFlag, multiredirectFlag, outputFile);
	}
	
	return;
}

/* Handles Updating of Paths
 * Takes in old path array , new Path array , new path array count
 */

void updatePath(char **path, char *newPaths[], int newPathsCount)
{
    // Firstly, clear the old paths
    for (int i = 0; i < path_count; i++)
    {
        // free(path[i]);
        path[i] = NULL;
    }

    // Copy the new paths into the array
    for (int i = 0; i < newPathsCount; i++)
    {
        int pathLen = strlen(newPaths[i]);
        char *str = newPaths[i];

        // Check if the path has a trailing slash, and if not, add one
        if (str[pathLen - 1] != '/')
        {
            // Allocate memory for the new path with a '/' added
            path[i] = malloc(pathLen + 2);  // +2 for the '/' and null terminator
            if (path[i] == NULL)
            {
                // Handle memory allocation error here
                exit(EXIT_FAILURE);
            }

            strcpy(path[i], str);         // Copy the original string
            strcat(path[i], "/");         // Add '/'
        }
        else
        {
            // No modification needed, just copy the original path
            path[i] = strdup(str);
        }
    }

    path_count = newPathsCount;
}

/* Handles running in parallel
* Split input by &
* For every command Format path and handleCommand
*/
// For & to run in parallel
void runParallel(char *input)
{

	char *commands[100];
	int commandIndex = 0;

	// Split input based on &
	char *command = strtok(input, "&");
	while (command != NULL)
	{
		
		commands[commandIndex] = command;
		commandIndex++;
		command = strtok(NULL, "&");
	}
	commands[commandIndex] = NULL;

	
	
    

	// Create child processes and execute each command in parallel
	for (int i = 0; i < commandIndex; i++)
	{
		char full_path[PATH_MAX_LENGTH];
		FormatPath(full_path, commands[i]);
		handleCommandls( commands[i],true);
		
	}

	// Parent process waits for all child processes to complete
    for (int j = 0; j < commandIndex; j++) {
        wait(NULL);
    }
	
}


// MAIN FUNCTION
int main(int MainArgc, char *MainArgv[])
{

	char *input = NULL; // Take in input
	size_t input_size = 0;

	char *dir = NULL;

	// Batch
	if (MainArgc == 2)
	{
		// Open batch file
		FILE *batch_file = fopen(MainArgv[1], "r");
		if (batch_file == NULL)
		{
			// Exit 1
			handleError();
			exit(EXIT_FAILURE);
		}

		while (getline(&input, &input_size, batch_file) != -1)
		{

			input = skipWhitespace(input);
			// Check if the line is empty
			if (strlen(input) == 0)
			{
				continue; // Skip empty lines
			}

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
					newPaths[newPathsCount] = token;
					newPathsCount++;
					
					token = strtok(NULL, " ");
				}

				updatePath(path, newPaths, newPathsCount);
			}

			// Non basic commmand Check for the "&" operator and run commands in parallel
			else if (strchr(input, '&') != NULL)
			{
				runParallel(input);

			}

			// Non basic commmand Single
			else
			{
				handleCommandls( input,false);
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

			input = skipWhitespace(input);
			// Check if the line is empty
			if (strlen(input) == 0)
			{
				continue; // Skip empty lines
			}

			// Exit 0 command
			if (strcmp(input, "exit") == 0)
			{
				// free(input);
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
				char *token = strtok(input + 5 , " ");

				while (token != NULL && newPathsCount < MAX_PATH_DIRECTORIES)
				{
					newPaths[newPathsCount] = token;
					newPathsCount++;
					token = strtok(NULL, " ");
				}

				updatePath(path, newPaths, newPathsCount);
			}
			// Non basic commmand Check for the "&" operator and run commands in parallel
			else if (strchr(input, '&') != NULL)
			{
				runParallel(input);

			}

			// Non Basic Commands
			else
			{
				
				handleCommandls( input,false);
			}
			
		}
		free(input);

	for (int i = 0; i < path_count; i++)
	{
		free(path[i]);
		path[i] = NULL;
	}
		
		return (0);
	}
}