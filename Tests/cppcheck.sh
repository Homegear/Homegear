#!/bin/bash
SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cppcheck $SCRIPTDIR/.. -I $SCRIPTDIR/../ARM\ headers/ -i rapidxml.hpp rapidxml_print.hpp
