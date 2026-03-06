//
// 3dblock.h
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
#ifndef _3dblock_h
#define _3dblock_h

#include <circle/2dgraphics.h>
#include <circle/display.h>

class C3DBlock		// 3D block (or cube), projected into a 2D graphics object
{
public:
	C3DBlock (double width = 1.0, double height = 1.0, double depth = 1.0);

	void SetRotation (double x, double y, double z);	// angles in rad

	void SetPosition (double x, double y, double z);

	void SetCamera (double yaw,	// camera rotation around y-axis in rad
			double fov,	// field of view in degrees
			double x = 0.0,
			double y = 0.0,
			double z = 5.0);

	void Draw (C2DGraphics *p2DGraphics, T2DColor color = CDisplay::White);

private:
	struct TVec3
	{
		double x, y, z;
	};

	struct TPoint
	{
		boolean visible;
		int x, y;
	};

	static TVec3 RotateX (const TVec3 &p, double a);
	static TVec3 RotateY (const TVec3 &p, double a);
	static TVec3 RotateZ (const TVec3 &p, double a);

	static TVec3 TranslateModel (const TVec3 &p, const TVec3 &t);
	static TVec3 TranslateToCamera (const TVec3 &camP, const TVec3 &camPos);

	static TPoint ProjectPersp (const TVec3 &p, double fovDeg, int width, int height);

private:
	TVec3 m_ModelPoint[8];
	TVec3 m_ModelRot;
	TVec3 m_ModelPos;

	TVec3 m_CamPos;
	double m_CamYaw;
	double m_CamFoV;

	static const unsigned s_ModelEdge[][2];		// index of start and end point
};

#endif
