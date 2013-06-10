#ifndef EXCEPTION_H
#define EXCEPTION_H

using namespace std;

#include <exception>
#include <string>

class Exception : std::exception
{
    public:
        /** Default constructor */
        Exception(std::string message);

        const char* what() const noexcept { return _message; }
    protected:
    private:
        const char* _message;
};

#endif // EXCEPTION_H
