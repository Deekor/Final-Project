#include <fstream>
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

using namespace std;


int main(int argc, char const *argv[])
{
    boost::property_tree::ptree pt;
    boost::property_tree::ptree pt1;
    boost::property_tree::ptree pt2;
    boost::property_tree::ptree pt3;

    pt1.put("name", "a2");
    pt1.put("contents", "adsf");

    pt2.put("name", "d6");
    pt2.put("contents", "345");

    pt3.put("name", "d2");
    pt3.put("contents", "=d6");

    pt.add_child("spreadsheet.cell", pt1);
    pt.add_child("spreadsheet.cell", pt2);
    pt.add_child("spreadsheet.cell", pt3);
    pt.put("spreadsheet.meta.password", "boobs");

    write_xml("output.xml", pt);

    BOOST_FOREACH(const boost::property_tree::ptree::value_type &v, pt.get_child("spreadsheet")) 
    {
        if(v.first == "meta")
        {
            string name = v.second.get<string>("password");
            cout << name << endl;
        }
    }
 
	
	return 0;
}