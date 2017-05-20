#ifndef DEBUG_PRINT
#define DEBUG_PRINT

#include <iostream>
#include <string>
#include <iomanip>
using std::string;
using std::cout; using std::endl;

const string red("\033[0;31m");
const string green("\033[1;32m");
const string yellow("\033[1;33m");
const string cyan("\033[0;36m");
const string magenta("\033[0;35m");
const string reset("\033[0m");

void inline print_success(string str) {
    cout << green << "\tsuccess\t" << reset << str << endl;
}

void inline print_failure(string str) {
    cout << red << "\tfailure\t" << reset << str << endl;
} 

#ifndef NDEBUG 

#define print_func()\
    cout <<  magenta << std::setw(30) << std::left << __func__ << reset  << '\t'

#define print_result(result)\
{\
    print_func();\
    if (result == VK_SUCCESS) {\
        cout << green << "success" << reset << endl;\
    } else {\
        cout << red   << "failure" << reset << endl;\
    }\
}\

#else 
#define print_func() 
#define print_result(arg) (void)arg
#endif 


#endif
