/******************************************************************************

csc 331 assignment 2: basic linux shell
Eric Lacker
ID 871024

fixed unable to run
*******************************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

void errorOut(){
    // output error message
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

int inputArguments(char* str){
    // reads arguments from stdin
    char* buffer = NULL;
    size_t bufferSize;
    
    if(getline(&buffer, &bufferSize, stdin) != -1){
        buffer[strcspn(buffer, "\n")] = 0; // remove trailing \n
        strcpy(str, buffer);
    }
    free(buffer);
    
    if (strcmp(str, "") == 0)
        return 0;
    else if (strstr(str, "&") != NULL)
        return 1;
    else return 2;
}

int returnParallelParsed(char* inputStr, char** parallelParsed){
    int i = 0, ret = 0;
    char *p = strtok(inputStr, "&");
    
    while (p != NULL){
        parallelParsed[i++] = p;
        p = strtok(NULL, "&");
        ret++;
    }
    return ret;
}

int parseCommands(char* inputStr, char** parsedArgs, char** redirectFile){
    // parses commands from input string to parsedArgs[], if > exists
    // return location of the char and set dest. file
    int i = 0, slot = -1, flag = 0;
    char *p = strtok(inputStr, " ");
    
    while (p != NULL){
        if (strstr(p, ">") != NULL && flag == 0 && i != 0){ //??
            slot = i;
            flag++;
        }
        if (flag == 1)
            redirectFile[0] = p;
        parsedArgs[i++] = p;
        p = strtok(NULL, " ");
    }
    return slot;
}

void makeRedirectArgs(char** parsedArgs, int redirectSlot, char** parsedRedirectArgs){
    // makes list of args up to '>'
    int i = 0;
    do {
        parsedRedirectArgs[i] = parsedArgs[i];
        i ++;
    } while (i < redirectSlot);
}

int builtInArg(char* arg, char** builtInCommands){ 
    // returns num of cmd if builtin, 0 if not
    int i = 0, flag = 0;
    
    for (i = 0; i < 3; i++){
        if (strcmp(arg, builtInCommands[i]) == 0){
            flag = i + 1;
        }
    }
    return flag;
}

int runBuiltInCommand(int commandNum, char** parsedArgs, char** path){
    int chdirint = 0, i = 1;
    
    switch (commandNum) { 
        case 1: // "exit"
            if (parsedArgs[1] == NULL)
                exit(0);
            else return 1;
            
        case 2: // "cd"
            if (parsedArgs[1] == NULL)
                return 1;
            else if (parsedArgs[2] != NULL)
                return 1;
            else if ((chdirint = chdir(parsedArgs[1])) == -1)
                return 1;   
            else return 0;
            
        case 3: // "path"
            memset(path, 0, 1000);
            while (parsedArgs[i] != NULL){
                path[i-1] = parsedArgs[i];
                i++;
            }
            return 0; 
        default: 
            break; 
    }      
    return 1;
}

int runArgs(char** parsedArgs, char** path, char** redirectFile, char** parsedRedirectArgs) { 
    // fork a process to run command (ret 0 for no error, 1 for error)
    int i = 0, k;
    char tempPath[1024];
    
    while (path[i] != NULL){
        snprintf(tempPath, 1024, "%s/%s", path[i], parsedArgs[0]);
        if ((k = access(tempPath, X_OK)) == 0)
            break;
        i++;
    }
    
    pid_t pid = fork();  
  
    if (pid == -1) { //fork failed
        return 1; 
    } 
    else if (pid == 0) { // fork successful
        if (redirectFile[0] == NULL){
            if (execv(tempPath, parsedArgs) < 0) 
                return 1; 
        exit(0); 
        } 
        else if (redirectFile[0] != NULL){
            FILE* fd = fopen(redirectFile[0], "w");
            
            if (fd == NULL)
                errorOut();

            dup2(fileno(fd), fileno(stdout));   //output to fd
            dup2(fileno(fd), fileno(stderr));   //stderr to fd
        
            fclose(fd);
            
            if (execv(tempPath, parsedRedirectArgs) < 0)
                return 1;
        exit(0);
        }
    } 
    else { 
        wait(NULL);  //wait for child process to finish
        return 0; 
    } 
    return 0;
} 

void shellMain(char *inputStr){
    char *parsedArgs[100], *builtInCommands[3], *path[1000],
            *parsedRedirectArgs[100], *redirectFile[1];
    
    // initialize int
    int commandNum = 5, redirectSlot = 0;
    
    //set list of builtin commands
    builtInCommands[0] = "exit";
    builtInCommands[1] = "cd";
    builtInCommands[2] = "path";
    
    //set initial path
    path[0] = "/bin";
    
    //parse input into parsedArgs
    redirectSlot = parseCommands(inputStr, parsedArgs, redirectFile);
    
    if (redirectSlot != -1)
        makeRedirectArgs(parsedArgs, redirectSlot, parsedRedirectArgs);
    else 
        redirectFile[0] = NULL;
    
    //check if file
    if (strstr(parsedArgs[0], ".") != NULL){
        if (parsedArgs[1] != NULL) // check if arg after file: error
            exit(1);
        
        char *buffer;
        size_t bufferSize = 0;
        
        // open file 
        FILE *fd = fopen(parsedArgs[0], "r");
        if (fd == NULL) exit(1);
        
        // loop through file and execute commands
        while (getline(&buffer, &bufferSize, fd) != -1){
            
            buffer[strcspn(buffer, "\n")] = 0; // remove trailing \n
            
            redirectSlot = parseCommands(buffer, parsedArgs, redirectFile);
            
            if (redirectSlot != -1)
                makeRedirectArgs(parsedArgs, redirectSlot, parsedRedirectArgs);
            else 
                redirectFile[0] = NULL;
            
            if ((commandNum = builtInArg(parsedArgs[0], builtInCommands)) != 0){
                if (runBuiltInCommand(commandNum, parsedArgs, path) == 1)
                    errorOut();
                return;
            }
            
            if (runArgs(parsedArgs, path, redirectFile, parsedRedirectArgs) == 1)
                errorOut();
            else if (redirectSlot != -1) printf("\n"); // formatting
        }
        exit(0); // exit(0) on complete
    }
    
    //check if builtin
    if ((commandNum = builtInArg(parsedArgs[0], builtInCommands)) != 0){
        if (runBuiltInCommand(commandNum, parsedArgs, path) == 1)
            errorOut();
        return;
    }
    
    //check if syscall works
    if (runArgs(parsedArgs, path, redirectFile, parsedRedirectArgs) == 1){
        errorOut();
    } else if (redirectSlot != -1) printf("\n"); // formatting
    
    //CLEAR DATA EXCEPT path, builtInCommands
    memset(inputStr, 0, 5000);
    memset(parsedArgs, 0, 100);
    memset(parsedRedirectArgs, 0, 100);
    memset(redirectFile, 0, 1);
}

void *shellMainHelper(void *voidInputArgs){
    char * inputStr = (char *)voidInputArgs;
    shellMain(inputStr);
    return NULL;
}

int main(){
    
    char inputStr[5000], *parallelParsed[2500];
    int flag = 0;

    while (1){
        //CLEAR DATA 
        memset(inputStr, 0, 5000);
        
        // print input marker 
        printf("grsh> ");
    
        // input args
        if ((flag = inputArguments(inputStr)) == 0){ // no command: loop
            continue;
        } 
        else if (flag == 1) { // parallel args, create threads
            pthread_t threads[500];
            int i, parNum, rc;
            
            if ((parNum = returnParallelParsed(inputStr, parallelParsed)) == 0)
                errorOut();
            
            // spawn the threads
            for (i = 0; i < parNum; ++i){
                if(pthread_create(&threads[i], NULL, shellMainHelper, (void *) parallelParsed[i])){
                    errorOut();
                }
            }
            
            // wait for threads to finish 
            for (i = 0; i < parNum; ++i)
                if(pthread_join(threads[i], NULL) != 0)
                    errorOut();
            continue;
        }
        else { // command present
            shellMain(inputStr);
        } 
    } // end while loop
}






