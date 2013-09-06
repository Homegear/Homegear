#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <exception>
#include <string>

class Exception : std::exception
{
    public:
        Exception(std::string message) {  _message = message; }

        virtual std::string what() { return _message; }
    protected:
        std::string _message;
};

#endif // EXCEPTION_H
