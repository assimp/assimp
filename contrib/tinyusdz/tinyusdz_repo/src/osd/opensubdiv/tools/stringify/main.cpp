//
//   Copyright 2013 Pixar
//
//   Licensed under the Apache License, Version 2.0 (the "Apache License")
//   with the following modification; you may not use this file except in
//   compliance with the Apache License and the following modification to it:
//   Section 6. Trademarks. is deleted and replaced with:
//
//   6. Trademarks. This License does not grant permission to use the trade
//      names, trademarks, service marks, or product names of the Licensor
//      and its affiliates, except as required to comply with Section 4(c) of
//      the License and to reproduce the content of the NOTICE file.
//
//   You may obtain a copy of the Apache License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the Apache License with the above modification is
//   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//   KIND, either express or implied. See the Apache License for the specific
//   language governing permissions and limitations under the Apache License.
//

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>


std::string stringify( std::string const & line ) {

    bool inconstant=false;

    std::stringstream s;
    for (int i=0; i<(int)line.size(); ++i) {

        // escape double quotes
        if (line[i]=='"') {
            s << '\\' ;
            inconstant = inconstant ? false : true;
        }

        if (line[i]=='\\' && line[i+1]=='\0') {
            s << "\"";
            return s.str();
        }

        // escape backslash
        if (inconstant && line[i]=='\\')
           s << '\\' ;

        s << line[i];
    }

    s << "\\n\"";

    return s.str();
}

int main(int argc, char **argv) {

    if (argc != 3) {
        std::cerr << "Usage: stringify input-file output-file" << std::endl;
        return 1;
    }

    std::ifstream input;
    input.open(argv[1]);
    if (! input.is_open()) {
        std::cerr << "Can not read from: " << argv[1] << std::endl;
        return 1;
    }

    std::ofstream output;
    output.open(argv[2]);
    if (! output.is_open()) {
        std::cerr << "Can not write to: " << argv[2] << std::endl;
        return 1;
    }

    std::string line;

    while (! input.eof()) {
        std::getline(input, line);
        output << "\"" << stringify(line) << std::endl;
    }

    return 0;
}
