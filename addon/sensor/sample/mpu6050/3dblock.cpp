//
// 3dblock.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2026  R. Stange <rsta2@gmx.net>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include "3dblock.h"
#include <math.h>
#include <assert.h>

#define PI			3.1415926

#define ARRAY_SIZE(array)	(sizeof (array) / sizeof ((array)[0]))

const unsigned C3DBlock::s_ModelEdge[][2] =
{
	{0, 1}, {1, 2}, {2, 3}, {3, 0},		// front square (z = -1)
	{4, 5}, {5, 6}, {6, 7}, {7, 4},		// back square (z = +1)
	{0, 4}, {1, 5}, {2, 6}, {3, 7}		// connections
};

C3DBlock::C3DBlock (double width, double height, double depth)
:	m_ModelRot {0.0, 0.0, 0.0},
	m_ModelPos {0.0, 0.0, 0.0},
	m_CamPos {0.0, 0.0, 5.0},
	m_CamYaw (0.0),
	m_CamFoV (60.0)
{
	double x = width / 2.0;
	double y = height / 2.0;
	double z = depth / 2.0;

	// end points of edges
	m_ModelPoint[0] = {-x, -y, -z};
	m_ModelPoint[1] = { x, -y, -z};
	m_ModelPoint[2] = { x,  y, -z};
	m_ModelPoint[3] = {-x,  y, -z};
	m_ModelPoint[4] = {-x, -y,  z};
	m_ModelPoint[5] = { x, -y,  z};
	m_ModelPoint[6] = { x,  y,  z};
	m_ModelPoint[7] = {-x,  y,  z};
}

void C3DBlock::SetRotation (double x, double y, double z)
{
	m_ModelRot = {x, y, z};
}

void C3DBlock::SetPosition (double x, double y, double z)
{
	m_ModelPos = {x, y, z};
}

void C3DBlock::SetCamera (double yaw, double fov, double x, double y, double z)
{
	m_CamYaw = yaw;
	m_CamFoV = fov;
	m_CamPos = {x, y, z};
}

void C3DBlock::Draw (C2DGraphics *p2DGraphics, T2DColor color)
{
	assert (p2DGraphics);
	int width = p2DGraphics->GetWidth ();
	int height = p2DGraphics->GetHeight ();

	// project all points
	const unsigned points = ARRAY_SIZE (m_ModelPoint);
	TPoint projected[points];
	for (unsigned i = 0; i < points; i++)
	{
		// model to world
		TVec3 p = m_ModelPoint[i];
		p = RotateZ (p, m_ModelRot.z);		// rotate z -> y -> x
		p = RotateY (p, m_ModelRot.y);
		p = RotateX (p, m_ModelRot.x);
		p = TranslateModel (p, m_ModelPos);

		// world to camera
		p = TranslateToCamera (p, m_CamPos);
		if (m_CamYaw != 0.0)
		{
			p = RotateY (p, m_CamYaw);
		}

		projected[i] = ProjectPersp (p, m_CamFoV, width, height);
	}

	// draw lines, if both end points are visible
	const unsigned edges = ARRAY_SIZE (s_ModelEdge);
	for (unsigned i = 0; i < edges; i++)
	{
		int start = s_ModelEdge[i][0];
		int end = s_ModelEdge[i][1];

		if (   projected[start].visible
		    && projected[end].visible)
		{
			p2DGraphics->DrawLine (projected[start].x, projected[start].y,
					       projected[end].x, projected[end].y,
					       color);
		}
	}
}

C3DBlock::TVec3 C3DBlock::RotateX (const TVec3 &p, double a)
{
	double c = cos(a), s = sin(a);
	return { p.x, p.y*c - p.z*s, p.y*s + p.z*c };
}

C3DBlock::TVec3 C3DBlock::RotateY (const TVec3 &p, double a)
{
	double c = cos(a), s = sin(a);
	return { p.x*c + p.z*s, p.y, -p.x*s + p.z*c };
}

C3DBlock::TVec3 C3DBlock::RotateZ (const TVec3 &p, double a)
{
	double c = cos(a), s = sin(a);
	return { p.x*c - p.y*s, p.x*s + p.y*c, p.z };
}

C3DBlock::TVec3 C3DBlock::TranslateModel (const TVec3 &p, const TVec3 &t)
{
	return { p.x + t.x, p.y + t.y, p.z + t.z };
}

C3DBlock::TVec3 C3DBlock::TranslateToCamera (const TVec3 &p, const TVec3 &camPos)
{
	return { p.x - camPos.x, p.y - camPos.y, p.z - camPos.z };
}

C3DBlock::TPoint C3DBlock::ProjectPersp (const TVec3 &camP, double fovDeg, int width, int height)
{
	if (camP.z >= 0.0001)
	{
		return { FALSE, 0, 0 };		// point in front of camera, which is not visible
	}

	double fovRad = fovDeg * PI / 180.0;
	double f = 1.0 / tan(fovRad / 2.0);

	double aspect = double (width) / double (height);

	double x_ndc = (camP.x * f / aspect) / -camP.z;
	double y_ndc = (camP.y * f) / -camP.z;

	int x = int ((x_ndc + 1.0) * 0.5 * width);
	int y = int ((1.0 - (y_ndc + 1.0) * 0.5) * height);

	return { TRUE, x, y };
}
