#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

int RunCmd(char *cmd, char*args[]){
	/* 没有输入命令 */
        if (!cmd || !*cmd)
            return 1;

        /* 内建命令 */
        if (strcmp(cmd, "cd") == 0) {
            if (args[0])
                chdir(args[1]);
            return 1; 
        }
        if (strcmp(cmd, "pwd") == 0) {
            char wd[4096];
            puts(getcwd(wd, 4096));
            return 1;
        }
        if (strcmp(cmd, "exit") == 0)
            return 0;

        // export to change env
        if (strcmp(cmd, "export") == 0){
        	char *name = args[1], *value = args[2];
        	char c, *check;
        	int i, tag_overwrite;
        	tag_overwrite = 1;

        	if(args[1] && args[2]){
        		check = getenv(args[1]);
        		if (check)
        		{
        			printf("Overwrite the enviromnet of %s = %s ?y/n\n ", args[1], check);
        			fflush(stdin);
        			scanf("%c\n", &c);
        			if (c == 'y')
        				tag_overwrite = 1;
        			else tag_overwrite = 0;
        		}
        		setenv(name, value, tag_overwrite);
        	}
        	else printf("Illegal path formatt\n");
        	return 1;
        }

        /* 外部命令 */
        pid_t pid = fork();
        if (pid == 0) {
            /* 子进程 */
            execvp(cmd, args);
            /* execvp失败 */
            return 255;
        }
        /* 父进程 */
        wait(NULL);
        return 1;
}

// int PrintAllArg(char *args[], int argsnum){
// 	int i;
// 	printf("PrintAllArg:\n");
// 	for (i = 0; i < argsnum; i++)
// 		printf("%d %s, ",i, args[i]);
// 	printf("\n");
// 	return 0;
// }

int main()
{
	char line[256], c;
	//memset(cmd, '\0', 256);
	char *cmd[128], *args[128][128];
	/* code */
	int i, j, tag_getting, tag_get_a_cmd, argsnum[128];
	while(1)
    {
		tag_getting = 0;
		tag_get_a_cmd = 0;

		printf("# ");
        fflush(stdin);
        fgets(line, 256, stdin);

		for( i = 0, j = 0 ; i < 256; i++)
        {
			if (line[i] == '|')
            {
				line[i] = '\0';
				if(tag_get_a_cmd == 1)
                {
					args[j][argsnum[j]] = NULL;
					j++;
				}
				tag_get_a_cmd = 0;
				tag_getting = 0;	
			}
			else{ 
				if (line[i] && line[i] != ' ' && line[i] != '\n' && line[i] != '=')
				{
					if (!tag_getting)
					{
						tag_getting = 1;

						if (!tag_get_a_cmd)
						{
							cmd[j] = &line[i];

							argsnum[j] = 0;
							args[j][argsnum[j]] = &line[i];
							argsnum[j]++;
							tag_get_a_cmd = 1;
						}
						else
						{
							args[j][argsnum[j]] = &line[i];
							argsnum[j]++;
						}

					}
				}
				else 
				{	
					c = line[i];
					args[j][argsnum[j]] = NULL;
					line[i] = '\0';
					tag_getting = 0;
					if (c == '\n')
					 	break;
				}
			}

		}


		int count = 0,state;
		pid_t pid;
		int fd[2], redirect_fd, tag_redirect;

		int oldstdout = dup(STDOUT_FILENO);
		int oldstdin = dup(STDIN_FILENO);
		pid = 0;
		for (count = 0; count <= j; count++)
        {
			if ( j - count > 0 && pid == 0) //pipe required
			{
				if (pipe(fd) == -1)
				{
					printf("pipe error\n");
					break;
				}
				else
                {
					pid = fork();
					if (pid < 0)
					{
						printf("fork error\n");
						break;
					}

					else{
						if (pid == 0)
						{
							close(fd[1]);
							dup2(fd[0], STDIN_FILENO);
							close(fd[0]);							
						}
						else
                        {
							fflush(stdout);
							dup2(fd[1], STDOUT_FILENO);
							close(fd[0]);
							state = RunCmd(cmd[count], args[count]);
							dup2(oldstdout, STDOUT_FILENO); 
							close(fd[1]);

							if(state != 1)
								return state;
							
							wait(NULL);
							
							break;
							
						}
					}
				}
			}
			else {
				for( i = 1, tag_redirect = 0; i < argsnum[count]; i++){
					if (strcmp(args[count][i], ">") == 0)
					{
						args[count][i] = NULL;
                        tag_redirect = 1;
                        redirect_fd = open(args[count][i + 1], O_CREAT | O_WRONLY | O_TRUNC);
                        dup2(redirect_fd, STDOUT_FILENO);
						break;
					}
					else
                    { 
                        if(strcmp(args[count][i], ">>") == 0)
                        {
						args[count][i] = NULL;
                        tag_redirect = 1;
                        redirect_fd = open(args[count][i + 1], O_CREAT | O_APPEND | O_WRONLY);
                        dup2(redirect_fd, STDOUT_FILENO);
						break;
                        }
                        else if (strcmp(args[count][i], "<") == 0)
                        {
                            char buffer[128];
                            struct stat statbuff;
                            args[count][i] = NULL;
                            tag_redirect = 1;
                            stat(args[count][i + 1], &statbuff);
                            redirect_fd = open(args[count][i + 1], O_RDWR); 

                            read(redirect_fd, buffer, statbuff.st_size - 1);
                        
                            fflush(stdin);
                            write(STDIN_FILENO, buffer, statbuff.st_size);
                            break;
                        }
					}
				}
				
				state = RunCmd(cmd[count], args[count]);
				
				dup2(oldstdout, STDOUT_FILENO);
				dup2(oldstdin, STDIN_FILENO);

				if (tag_redirect == 1)
                {
                    close(redirect_fd);
                }
                


				fflush(stdin);
				fflush(stdout);

				if(state == 0)
					return state;
				if(state == 255)
					printf("Command not found or execution failed\n");
				break;

			}
		}
	}
	return 0;
}