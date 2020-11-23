#include <vector>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sstream>
#include <iostream>

#include "linenoise.h"

#include "debugger.hpp"

using namespace minidbg;

std::vector<std::string> split(const std::string &s, char delimiter) {
    std::vector<std::string> out{};
    std::stringstream ss {s};
    std::string item;

    while (std::getline(ss,item,delimiter)) {
        out.push_back(item);
    }

    return out;
}

bool is_prefix(const std::string& s, const std::string& of) {
    if (s.size() > of.size()) return false;
    return std::equal(s.begin(), s.end(), of.begin());
}

//Handling input

//Our commands will follow a similar format to gdb and lldb. 
//To continue the program, a user will type continue or cont or even just c. 
//If they want to set a breakpoint on an address, they’ll write break 0xDEADBEEF, where 0xDEADBEEF is the desired address in hexadecimal format.
// Let’s add support for these commands.

void debugger::handle_command(const std::string& line) {
    auto args = split(line,' ');
    auto command = args[0];

    if (is_prefix(command, "cont")) {
        continue_execution();
    }
    else {
        std::cerr << "Unknown command\n";
    }
}

//In our run function, we need to wait until the child process has finished launching, then keep on getting input from linenoise until we get an EOF (ctrl+d)

void debugger::run() {
    int wait_status;
    auto options = 0;
    waitpid(m_pid, &wait_status, options);

    char* line = nullptr;
    while((line = linenoise("minidbg> ")) != nullptr) {
        handle_command(line);
        linenoiseHistoryAdd(line);
        linenoiseFree(line);
    }
}

void debugger::continue_execution() {
    //ptrace allows us to observe and control the execution of another process by reading registers, reading memory, single stepping and more
    ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);

    int wait_status;
    auto options = 0;
    waitpid(m_pid, &wait_status, options);
}

void execute_debugee (const std::string& prog_name) {
    //running child process 
    if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
        std::cerr << "Error in ptrace\n";
        return;
    }
    //We execute the given program, passing the name of it as a command-line argument and a nullptr to terminate the list
    execl(prog_name.c_str(), prog_name.c_str(), nullptr);
}

//main class
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Program name not specified";
        return -1;
    }

    auto prog = argv[1];

    auto pid = fork();
    if (pid == 0) {
        //child
        execute_debugee(prog);

    }
    else if (pid >= 1)  {
        //parent
        std::cout << "Started debugging process " << pid << '\n';
        //debugger for interacting with child process
        debugger dbg{prog, pid};
        dbg.run();
    }
}
//In our run function, we need to wait until the child process has finished launching, then keep on getting input from linenoise until we get an EOF (ctrl+d)

