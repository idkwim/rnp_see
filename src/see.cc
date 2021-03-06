/*
    Copyright 2012 Alex Eubanks (endeavor[at]rainbowsandpwnies.com)

    This file is part of rnp_see ( http://github.com/endeav0r/rnp_see/ )

    rnp_see is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <list>
#include <map>
#include <sstream>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <udis86.h>

#include "elf.h"
#include "instruction.h"
#include "lx86.h"
#include "translator.h"

#include "vm.h"

void help (std::string name)
{
    std::cout << "Usage: " << name << " <loader> <filename>" << std::endl;
    std::cout << "   Loader: You must specify a loader" << std::endl;
    std::cout << "   --elf    attempts to load the binary directly from the elf" << std::endl;
    std::cout << "   --lx86   forks the x86 linux process, breaks at entry, and loads" << std::endl;
}

int main (int argc, char * argv[])
{
    int loader_type = 0;
    int option_index = 0;

    struct option options [] = {
        {"lx86", no_argument, &loader_type, 1},
        {"elf",  no_argument, &loader_type, 2}
    };

    while (true) {
        int c = getopt_long(argc, argv, "", options, &option_index);
        if (c == -1) break;

        switch(c) {
            case '?' :
                help(argv[0]);
                return -1;
        }
    }

    if (loader_type == 0) {
        help(argv[0]);
        return -1;
    }

    Loader * loader;

    if (loader_type == 1)
        loader = new Lx86(argv[optind]);
    else
        loader = Elf::Get(argv[optind]);

    Engine engine(loader);

    std::cout << std::endl;

    while (true) {
        int c = getc(stdin);
        if (c == 'a') { while (true) engine.step(); }
        if (c == 'd') { for (int i = 0; i < 8; i++) engine.step(); }
        if (c == 'f') { for (int i = 0; i < 16; i++) engine.step(); }
        if (c == 'g') { for (int i = 0; i < 128; i++) engine.step(); }
        if (c == 'h') { for (int i = 0; i < 1024; i++) engine.step(); }
        if (c == 'q') break;
        //if (c == 'r') engine.debug_x86_registers();
        if (c == 's') engine.step();
        if (c == 't') { for (int i = 0; i < 500; i++) engine.step(); break; }
        //if (c == 'v') vm.debug_variables();
    }

    delete loader;

    return 0;
}
