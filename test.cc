#include <fstream>
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <sstream>

using namespace std;


int main(int argc, char const *argv[])
{
    boost::property_tree::ptree pt;

    pt.put("spreadsheet.cell.name", "a2");
    pt.put("spreadsheet.cell.contents", "adsf");

    write_xml("output.xml", pt);

    boost::property_tree::ptree ptr;
    read_xml("output.xml", ptr);

    ptr.put("spreadsheet.cell.name", "d6");
    ptr.put("spreadsheet.cell.contents", "345");
    ptr.put("spreadsheet.cell.name", "d2");
    ptr.put("spreadsheet.cell.contents", "=d6");

    write_xml("output2.xml", ptr);
	
	return 0;
}