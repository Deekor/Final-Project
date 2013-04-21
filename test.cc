#include <fstream>
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <sstream>
#include <boost/foreach.hpp>

using namespace std;


int main(int argc, char const *argv[])
{
    boost::property_tree::ptree pt;
    boost::property_tree::ptree pt1;
    boost::property_tree::ptree pt2;
    boost::property_tree::ptree pt3;

    pt1.put("spreadsheet.cell.name", "a2");
    pt1.put("spreadsheet.cell.contents", "adsf");

    pt2.put("spreadsheet.cell.name", "d6");
    pt2.put("spreadsheet.cell.contents", "345");

    pt3.put("spreadsheet.cell.name", "d2");
    pt3.put("spreadsheet.cell.contents", "=d6");

    pt.add_child("spreadsheet.cell", pt1);
    pt.add_child("spreadsheet.cell", pt2);
    pt.add_child("spreadsheet.cell", pt3);

    write_xml("output.xml", pt);

    BOOST_FOREACH(const boost::property_tree::ptree::value_type &v, pt.get_child("spreadsheet")) 
    {
       string name = v.second.first.get_value("name");
       cout << name << endl;
    }
	
	return 0;
}