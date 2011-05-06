#!/bin/sh
grep -E 'IFC::Ifc([A-Z][a-z]*)+' -o ../../code/IFCLoader.cpp | uniq | sed s/IFC::// > output.txt
