//
// enumerator.hxx
//
// Angle - A C++ template library for Circle
// Copyright (C) 2014 Bidon Aurelien <bidon.aurelien@gmail.com>
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

#ifndef _angle_enumerator_h
#define _angle_enumerator_h

namespace angle {
	template <typename _type>
	class Enumerator
	{
	public:
		virtual ~Enumerator() { }
		
		virtual bool
		hasNext() const = 0;
		
		virtual bool
		hasPrevious() const = 0;
		
		virtual _type*
		next() = 0;
		
		virtual _type*
		previous() = 0;
	};
	
	template <typename _type>
	class Enumerable
	{
	public:
		virtual angle::Enumerator<const _type>*
		constEnumerator() const = 0;
		
		virtual angle::Enumerator<const _type>*
		constRevEnumerator() const = 0;
		
		virtual angle::Enumerator<_type>*
		enumerator() = 0;
		
		virtual angle::Enumerator<_type>*
		revEnumerator() = 0;
	};
}

#endif
