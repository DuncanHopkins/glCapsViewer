/*
*
* OpenGL hardware capability viewer and database
*
* OpenGL capability viewer core
*
* Copyright (C) 2011-2015 by Sascha Willems (www.saschawillems.de)
*
* This code is free software, you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License version 3 as published by the Free Software Foundation.
*
* Please review the following information to ensure the GNU Lesser
* General Public License version 3 requirements will be met:
* http://opensource.org/licenses/lgpl-3.0.html
*
* The code is distributed WITHOUT ANY WARRANTY; without even the
* implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
* PURPOSE.  See the GNU LGPL 3.0 for more details.
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <time.h> 

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <ctime>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <rapidxml.hpp>
#include <rapidxml_print.hpp>

#include <glCapsViewerCapsGroup.h>
#include "glCapsViewerCore.h"

using namespace std;

/// <summary>
/// Checks if an extension is supported (GL or os specific)
/// </summary>
/// <param name="ext">Name of the extension to check</param>
/// <returns>true if extension is present</returns>
bool glCapsViewerCore::extensionSupported(string ext) 
{
	for (auto& currExt : extensions) {
		if (ext == currExt) {
			return true;
		}
	}

	for (auto& currExt : osextensions) {
		if (ext == currExt) {
			return true;
		}
	}

	return false;
}

void glCapsViewerCore::clear() {
	extensions.clear();
	osextensions.clear();
	implementation.clear();
	capgroups.clear();
	description = "";
	submitter = "";	
}

/// <summary>
/// Reads the name of the operating system.
/// </summary>
/// <returns>Short name of the operating system</returns>
// TODO : Linux and Mac OSX
string glCapsViewerCore::readOperatingSystem()
{
	#ifdef _WIN32
		HKEY key;
		if (RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\"), &key) == ERROR_SUCCESS) {
			TCHAR value[MAX_PATH];
			DWORD type = REG_SZ;
			DWORD value_length = MAX_PATH;
			RegQueryValueEx(key, L"ProductName", NULL, &type, (LPBYTE)&value, &value_length);
			value[value_length] = wchar_t(0);
			char sz[1024] = { '\0' };
			wcstombs(sz, value, sizeof(sz));
			string osname = sz;
			RegCloseKey(key);
			// TODO : Clean up os name
			if (osname.find("Windows 10") != string::npos)
				return "Windows 10";
			if (osname.find("Windows 8") != string::npos)
				return "Windows 8";
			if (osname.find("Windows 7") != string::npos)
				return "Windows 7";
			if (osname.find("Vista") != string::npos)
				return "Windows Vista";
			if (osname.find("XP") != string::npos)
				return "Windows XP";
			if (osname.find("2000") != string::npos)
				return "Windows 2000";
			return "Windows (unknown)";
		} else {
			return "Windows (unknown)";
		}
	#else
		// TODO 
		return "unknown";
	#endif
}

void glCapsViewerCore::readExtensions()
{
	const GLubyte* glExtensions = glGetString(GL_EXTENSIONS);
	string extensionString = reinterpret_cast<const char*>(glExtensions);
	boost::algorithm::split(extensions, extensionString, boost::algorithm::is_any_of(" "));
}

void glCapsViewerCore::printExtensions()
{
	cout << "gl extensions (" << extensions.size() << ") : \n";
	for (auto& s : extensions)
		std::cout << '"' << s << '"' << '\n';
}

void glCapsViewerCore::readOsExtensions() 
{
	typedef const char* (WINAPI * PFNWGLGETEXTENSIONSSTRINGARBPROC)(HDC hdc);
	PFNWGLGETEXTENSIONSSTRINGARBPROC  pwglGetExtensionsStringARB = 0;
	#define wglGetExtensionsStringARB pwglGetExtensionsStringARB

	wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
	if (wglGetExtensionsStringARB)
	{
		const char* wglExtensions = wglGetExtensionsStringARB(wglGetCurrentDC());
		if (wglExtensions)
		{
			std::string wglExtensionString = reinterpret_cast<const char*>(wglExtensions);
			boost::algorithm::split(osextensions, wglExtensionString, boost::algorithm::is_any_of(" "));
		}
	}
}

void glCapsViewerCore::readImplementation()
{
	implementation["Operating system"] = readOperatingSystem();
	implementation["Vendor"] = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
	implementation["Renderer"] = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	implementation["OpenGL version"] = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	implementation["Shading language version"] = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION_ARB));

	stringstream ss;
	ss << implementation["Vendor"] << " " << implementation["Renderer"] << " " << implementation["OpenGL version"] << " (" << implementation["Operating system"] << ")";
	description = ss.str();

	// Not visible, but will be exported to xml
	glCapsViewerCapsGroup capsGroup;
	capsGroup.name = "implementation";
	capsGroup.supported = true;
	capsGroup.visible = false;
	capsGroup.addCapability("GL_VENDOR", GL_VENDOR, "glstring", 1);
	capsGroup.addCapability("GL_RENDERER", GL_RENDERER, "glstring", 1);
	capsGroup.addCapability("GL_VERSION", GL_VERSION, "glstring", 1);
	capsGroup.addCapability("GL_SHADING_LANGUAGE_VERSION", GL_SHADING_LANGUAGE_VERSION, "glstring", 1);
	capgroups.push_back(capsGroup);
}

using namespace boost::property_tree;

string glCapsViewerCore::reportToXml() 
{
	const string appVersion = "glCapsViewer 1.0(beta) - Copyright 2011-2015 by Sascha Willems (www.saschawillems.de)";
	const string appAbout = "glCapsViewer 1.0(beta)";
	const string fileVersion = "3.0";

	using namespace rapidxml;
	xml_document<> doc;
	xml_node<> *root = doc.allocate_node(node_element, "implementationinfo");
	doc.append_node(root);
	root->append_node(doc.allocate_node(node_element, "fileversion", fileVersion.c_str()));
	root->append_node(doc.allocate_node(node_element, "appversion", appVersion.c_str()));
	root->append_node(doc.allocate_node(node_element, "description", description.c_str()));
	root->append_node(doc.allocate_node(node_element, "contexttype", contextType.c_str()));

	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	char buffer[256];
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
	string datetimestr = reinterpret_cast<const char*>(buffer);
	root->append_node(doc.allocate_node(node_element, "date", datetimestr.c_str()));

	root->append_node(doc.allocate_node(node_element, "submitter", submitter.c_str()));
	root->append_node(doc.allocate_node(node_element, "os", implementation["Operating system"].c_str()));

	// extensions
	xml_node<> *extNode = doc.allocate_node(node_element, "extensions");
	string extCount = to_string(extensions.size() + osextensions.size());
	extNode->append_attribute(doc.allocate_attribute("count", extCount.c_str()));
	root->append_node(extNode);
	for (auto& ext : extensions) {
		extNode->append_node(doc.allocate_node(node_element, "extension", ext.c_str()));
	}

	// implementation info and caps
	xml_node<> *capsNode = doc.allocate_node(node_element, "caps");
	root->append_node(capsNode);
	for (auto& capgroup : capgroups) {
		for (auto& cap : capgroup.capabilities) {
			xml_node<> *capNode = doc.allocate_node(node_element, cap.first.c_str());
			capNode->append_attribute(doc.allocate_attribute("id", cap.first.c_str()));
			capNode->append_node(doc.allocate_node(node_element, "value", cap.second.c_str()));
			capsNode->append_node(capNode);
		}
	}

	stringstream ss;
	ss << doc;
	return ss.str();
}

void glCapsViewerCore::exportXml(string fileName)
{
	string xml = reportToXml();
	ofstream destfile;
	destfile.open(fileName);
	destfile << xml;
	destfile.close();
}

template <typename ElemT>
struct HexTo {
	ElemT value;
	operator ElemT() const { return value; }
	friend std::istream& operator>>(std::istream& in, HexTo& out) {
		in >> std::hex >> out.value;
		return in;
	}
};

void glCapsViewerCore::readCapabilities()
{

	using namespace rapidxml;
	xml_document<> doc;
	xml_node<> * root_node;
	// Read the xml file into a vector
	ifstream theFile("capslist.xml");
	vector<char> buffer((istreambuf_iterator<char>(theFile)), istreambuf_iterator<char>());
	buffer.push_back('\0');
	doc.parse<0>(&buffer[0]);
	root_node = doc.first_node("categories");

	for (xml_node<> * category_node = root_node->first_node("category"); category_node; category_node = category_node->next_sibling())
	{
		string catName = category_node->first_attribute("name")->value();
		glCapsViewerCapsGroup capsGroup;
		capsGroup.name = catName;
		capsGroup.supported = true;

		// TODO : Check requirement (gl version, extension)
		// TODO : wgl and glx need to be checked different (wglewIsSupported, etc.)
		string reqExt = category_node->first_node("requirements")->first_attribute("extension")->value();
		string reqVersion = category_node->first_node("requirements")->first_attribute("version")->value();

		// Check extension
		if (reqExt != "") {
			capsGroup.supported = extensionSupported(reqExt);
		}
		if (reqVersion != "") {
			// TODO : Check against supported OpenGL version
		}

		if (capsGroup.supported) {
			for (xml_node<> * cap_node = category_node->first_node("cap"); cap_node; cap_node = cap_node->next_sibling("cap"))
			{
				string capName = cap_node->first_attribute("name")->value();
				string capType = cap_node->first_attribute("type")->value();
				uint32_t capComponents = boost::lexical_cast<HexTo<uint32_t>>(cap_node->first_attribute("components")->value());
				uint32_t capEnum = boost::lexical_cast<HexTo<uint32_t>>(cap_node->first_attribute("enum")->value());
				capsGroup.addCapability(capName, capEnum, capType, capComponents);
			}
		}

		capgroups.push_back(capsGroup);

	}

}