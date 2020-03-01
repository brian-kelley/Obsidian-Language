//Include this file in any testing main
#include "Common.hpp"
//TODO: get this to work on win32 :(
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

//Run the compiler with the given string of arguments,
//and piping the given string into its stdin.
//
//Return everything from its stdout, and set "crash" to true if the compiler
//didn't exit() normally.
string runOnyx(const vector<string>& args, string pipeIn, bool& crash)
{
  pid_t pid = 0;
  int compilerIn[2];
  int compilerOut[2];
  //note: first fd is read end, second fd is write end
  if(pipe(compilerIn) < 0)
  {
    cerr << "Failed to open pipe for compiler stdin.\n";
    exit(1);
  }
  if(pipe(compilerOut) < 0)
  {
    cerr << "Failed to open pipe for compiler stdout/stderr.\n";
    exit(1);
  }
  pid = fork();
  if(pid < 0)
  {
    perror("Failed to fork() the testing driver.\n");
    exit(0);
  }
  else if(pid == 0)
  {
    // child process (compiler):
    if(dup2(compilerIn[0], STDIN_FILENO) == -1)
    {
      cerr << "On child, failed to redirect stdin to pipe.\n";
      exit(1);
    }
    if(dup2(compilerOut[1], STDOUT_FILENO) == -1)
    {
      cerr << "On child, failed to redirect stdout to pipe.\n";
      exit(1);
    }
    if(dup2(compilerOut[1], STDERR_FILENO) == -1)
    {
      cerr << "On child, failed to redirect stderr to pipe.\n";
      exit(1);
    }
    close(compilerIn[0]);
    close(compilerIn[1]);
    close(compilerOut[0]);
    close(compilerOut[1]);
    const char* compiler = "../onyx";
    vector<const char*> rawArgs;
    rawArgs.push_back(compiler);
    for(auto& arg : args)
      rawArgs.push_back(arg.c_str());
    //finally, pass null pointer representing the end
    rawArgs.push_back(NULL);
    execve(compiler, (char*const*) rawArgs.data(), NULL);
    //done
    exit(0);
  }
  close(compilerIn[0]);
  close(compilerOut[1]);
  //Write pipeIn to the input pipe
  if(pipeIn.length())
    write(compilerIn[1], pipeIn.c_str(), pipeIn.length());
  //close the handle to mark "eof"
  close(compilerIn[1]);
  //Read all bytes of ouptut
  string pipeOut;
  char outbuf[4097];
  while(true)
  {
    int bytesRead = read(compilerOut[0], outbuf, 4096);
    if(bytesRead == 0)
      break;
    //otherwise, append the buf (as terinated string)
    //to full output string
    outbuf[bytesRead] = 0;
    pipeOut += outbuf;
  }
  int statLoc;
  wait(&statLoc);
  close(compilerOut[0]);
  crash = WTERMSIG(statLoc);
  return pipeOut;
}

bool compilerInternalError(const string& output)
{
  return output.find("<!> INTERNAL ERROR") != string::npos;
}

bool compilerNormalError(const string& output)
{
  return output.find("Error") != string::npos;
}

bool compilerSuccess(const string& output)
{
  return !compilerInternalError(output) &&
    !compilerNormalError(output);
}

void seedRNG()
{
  srand(clock());
}

