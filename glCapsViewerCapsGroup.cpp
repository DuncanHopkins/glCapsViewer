/*
*
* OpenGL hardware capability viewer and database
*
* OpenGL capability group implementation
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

#include "glCapsViewerCapsGroup.h"

/// <summary>
/// Execute http get request 
/// </summary>
/// <param name="idstr">OpenGL enum as string</param>
/// <param name="id">OpenGL enum to query</param>
/// <param name="type">Internal OpenGL enum type (glint, glstring)</param>
/// <param name="dim">No of dimensions to query</param>
void glCapsViewerCapsGroup::addCapability(string idstr, GLenum id, string type, int dim)
{
	if (type == "glint") {
		GLint* intVal;
		intVal = new GLint[dim];
		glGetIntegerv(id, intVal);
		string valString = "";
		for (int i = 0; i < dim; i++) {
			if (i > 0) {
				valString += " ,";
			}
			valString += to_string(intVal[i]);
		}
		GLint glerr = glGetError();
		capabilities[idstr] = valString;
		if (glerr != GL_NO_ERROR) {
			capabilities[idstr] = "0";
		}
		delete[] intVal;
	}

	if (type == "glstring") {
		string valString = reinterpret_cast<const char*>(glGetString(id));
		GLint glerr = glGetError();
		capabilities[idstr] = valString;
		if (glerr != GL_NO_ERROR) {
			capabilities[idstr] = "";
		}
	}
}