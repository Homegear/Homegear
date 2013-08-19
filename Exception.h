#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <exception>
#include <string>

class Exception : std::exception
{
    public:
        /** Default constructor */
        Exception(std::string message);

        std::string what() { return _message; }
    protected:
    private:
        std::string _message;
};

#endif // EXCEPTION_H
