#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include<fcntl.h>



char sysCmdPath[200]="/bin /usr/bin";

char *buildinCmd[]={"exit","help","cd","path"};

int main(int argc, const char * argv[]) {
    void ExecCommand(char **cmds,char *filename,bool redir);
    char **SplitCommand(char *line,char *delim);
    void ErrorMessage(char *errMsg);
    void CommandProcess(char *line);
    
    
    if(argv[1]!=NULL){
        
        if(argv[2]!=NULL){
            ErrorMessage("Invoke more than one file.");
            exit(1);
        }
        char const* const filename=argv[1];
        FILE *file=fopen(filename, "r");
        char line[256];
        
        if(file==NULL){
            ErrorMessage("read file error!");
            exit(1);
        }
        
        while(fgets(line, sizeof(line), file)){
            
            CommandProcess(line);
        }
        
        fclose(file);
        exit(0);
    }
    
    
    
    while(1){
        printf("dash> ");
        
        char *line = NULL;
        size_t len=0;
        getline(&line,&len,stdin);
        
        
        CommandProcess(line);
        
    }
    
    return 0;
}



char **SplitCommand(char *line,char *delim){
    int bufSize=100;
    
    char **cmds=malloc(bufSize*sizeof(char *));
    char *cmd=NULL;
    
    int i=0;
    cmd=strtok(line,delim);
    
    while(cmd!=NULL){
        cmds[i]=cmd;
        i++;
        cmd=strtok(NULL,delim);
    }
    cmds[i]=NULL;
    return cmds;
}



void ErrorMessage(char *errMsg){
    if(strcmp(errMsg, "")!=0){
        perror(errMsg);
    }
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}



void ExecCommand(char **cmds,char *filename,bool redir){
    
    int cmdLen=sizeof(buildinCmd)/sizeof(buildinCmd[0]);
    
    int buildinCmdCode=-1;
    int i;
    for(i=0;i<cmdLen;i++){
        if(strcasecmp(buildinCmd[i],cmds[0])==0){
            buildinCmdCode=i;
            break;
        }
    }
    

    if(buildinCmdCode!=-1){
        switch (buildinCmdCode) {
            case 0:
                
                if(cmds[1]!=NULL){
                    ErrorMessage("exit cannot have any arguments!");
                }
                else{
                    exit(0);
                }
                break;
            case 1:
                
                printf("%s","**** This is a help function ****\n");
                printf("%s","We provide below build in functions:\n");
                for(i=0;i<cmdLen;i++){
                    printf("%s ",buildinCmd[i]);
                }
                printf("\n");
                break;
            case 2:
            
                if(cmds[1]==NULL){
                    ErrorMessage("CD cannot find any argument!");
                }
                else if(cmds[2]!=NULL){
                    ErrorMessage("CD command can only take one argument!");
                }
                else {
                    if(chdir(cmds[1])!=0){
                        ErrorMessage("dash error");
                    }
                }
                break;
            case 3:
            
                strcpy(sysCmdPath, "");
                int j=1;
                while(cmds[j]!=NULL){
                    strcat(sysCmdPath, cmds[j]);
                    strcat(sysCmdPath," ");
                    j++;
                }
                break;
            default:
                break;
        }
    }
    

    else{
        int status;
        pid_t pid = fork();

        
        if (pid == 0) {
            
            char **cmdPaths=SplitCommand(sysCmdPath, " \t\r\n\a");
            bool findPath=false;
            
            int i=0;
            char *cmdPath=NULL;
            while(cmdPaths[i]!=NULL){
                cmdPath=(char *)malloc(strlen(cmdPaths[i])+strlen(cmds[0])+1);
                strcpy(cmdPath, cmdPaths[i]);
                strcat(cmdPath, "/");
                strcat(cmdPath, cmds[0]);
                
                
                if(access(cmdPath, X_OK)==0){
                    findPath=true;
                    
                    if(!redir){
                        if(execv(cmdPath, cmds)==-1){
                            ErrorMessage("dash error");
                            exit(EXIT_FAILURE);
                        }
                    }
                    break;
                }
                free(cmdPath);
                i++;
            }
            
            if(redir){
                int fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                dup2(fd, 1);
                dup2(fd, 2);
                close(fd);
                if(findPath){
                    execv(cmdPath,cmds);
                }
            }
            
            if(!findPath){
                ErrorMessage("Invalid Command! Please check your input or searching path!");
                exit(EXIT_FAILURE);
            }
        }
        
      
        else if (pid < 0) {
            ErrorMessage("dash fork error");
        }
        

        else {
            do {
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
    }
}



void CommandProcess(char *line){
    char **redCmds=NULL;
    char **parall=NULL;
    char **cmds=NULL;
    
    
    parall=SplitCommand(line, "&");
    
    int i=0;
    
    while(parall[i]!=NULL){
        cmds=NULL;
        
        char *redErr=strstr(parall[i], ">>");
        if(redErr){
            printf("%s","Error: Multiple redirection operators!\n");
            ErrorMessage("");
            break;
        }
        
        redCmds=SplitCommand(parall[i], ">");
        if(redCmds[1]!=NULL && redCmds[2]!=NULL){
            printf("%s","Error: Multiple redirection operators!\n");
            ErrorMessage("");
            break;
        }
        
        else if(redCmds[1]!=NULL){
            char **filename=SplitCommand(redCmds[1], " \t\r\n\a");
            //check redirection command error.
            if(filename[1]!=NULL){
                printf("%s","Error: Multiple redirection files!\n");
                ErrorMessage("");
                break;
            }
            cmds=NULL;
            cmds=SplitCommand(redCmds[0]," \t\r\n\a");
            if(cmds[0]!=NULL){
                ExecCommand(cmds, filename[0], true);
            }
        }
        
        else{
            cmds=NULL;
            cmds=SplitCommand(redCmds[0]," \t\r\n\a");
            if(cmds[0]!=NULL){
                ExecCommand(cmds, NULL, false);
            }
        }
        i++;
    }
}
