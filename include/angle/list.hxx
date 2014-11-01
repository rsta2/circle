//
// list.hxx
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

#ifndef _angle_list_h
#define _angle_list_h

#include <assert.h>
#include <angle/enumerator.hxx>

namespace angle {
	template <typename _type>
	class List : public Enumerable<_type>
	{
		struct Node {
			Node* next;
			Node* prev;
			_type value;
			
			Node(const _type& val);
			Node(const Node& copy);
			Node(const Node& copy, const Node& next, const Node& prev = 0);
			
			Node&
			operator=(const Node&) = delete;
		};
		
		template <typename _enumerated_type>
		class Enum : public Enumerator<_enumerated_type>
		{
		public:
			const List<_type>& _c;
			bool _e, _f;
			Node* _n;
			bool _reversed;
			
			Enum(const List<_type>& l, bool reversed);
			
			virtual bool
			hasNext(void) const;
			
			virtual bool
			hasPrevious(void) const;
			
			virtual _enumerated_type*
			next(void);
			
			virtual _enumerated_type*
			previous(void);
		};
		
		Node* _head;
		Node* _tail;
		
	public:
		List(void);
		List(const List& copy);
		List& operator=(const List& copy) = delete;

		_type&
		back(void);

		const _type&
		back(void) const;
		
		virtual angle::Enumerator<const _type>*
		constEnumerator(void) const;
		
		virtual angle::Enumerator<const _type>*
		constRevEnumerator(void) const;
		
		bool
		empty(void) const;
		
		virtual angle::Enumerator<_type>*
		enumerator(void);

		_type&
		front(void);

		const _type&
		front(void) const;
		
		void
		insert(const _type& val, const angle::Enumerator<_type>* pos);
		
		List<_type>&
		popBack(void);
		
		List<_type>&
		popFront(void);
		
		List<_type>&
		pushBack(const _type& v);
		
		List<_type>&
		pushFront(const _type& v);
		
		void
		remove(const angle::Enumerator<_type>* pos);
		
		virtual angle::Enumerator<_type>*
		revEnumerator(void);
	};
	
	/* List implementation */
	template <typename _type>
	List<_type>::List(void) :
	_head(0),
	_tail(0)
	{ }
	
	template <typename _type>
	List<_type>::List(const List<_type>& todo) :
	_head(0),
	_tail(0)
	{ assert(&todo == 0); }
	
	template <typename _type>
	_type&
	List<_type>::back(void)
	{
		assert(empty() == false);
		return _tail->value;
	}
	
	template <typename _type>
	const _type&
	List<_type>::back(void) const
	{
		assert(empty() == false);
		return _tail->value;
	}
	
	template <typename _type>
	angle::Enumerator<const _type>*
	List<_type>::constEnumerator(void) const
	{
		return new Enum<const _type>(*this, false);
	}
	
	template <typename _type>
	angle::Enumerator<const _type>*
	List<_type>::constRevEnumerator(void) const
	{
		return new Enum<const _type>(*this, true);
	}
	
	template <typename _type>
	bool
	List<_type>::empty(void) const
	{
		return _head == 0;
	}
	
	template <typename _type>
	angle::Enumerator<_type>*
	List<_type>::enumerator(void)
	{
		return new Enum<_type>(*this, false);
	}
	
	template <typename _type>
	_type&
	List<_type>::front(void)
	{
		assert(empty() == false);
		return _head->value;
	}
	
	template <typename _type>
	const _type&
	List<_type>::front(void) const
	{
		assert(empty() == false);
		return _head->value;
	}
	
	template <typename _type>
	void
	List<_type>::insert(const _type& val, const angle::Enumerator<_type>* e)
	{
		assert(e != 0);
		const List<_type>::Enum<_type>* pos = (const List<_type>::Enum<_type>*)e;
		assert(&pos->_c == this);
		
		pos->hasNext(); // used here to refresh the _n member if needed
		
		if (empty() || pos->_n == _tail)
		{
			pushBack(val);
			return;
		}
		
		Node* newNode = new Node(val);
		newNode->next = pos->_n->next;
		newNode->prev = pos->_n;
		pos->_n->next = newNode;
		newNode->next->prev = newNode;
	}

	template <typename _type>
	List<_type>&
	List<_type>::popBack(void) {
		assert(empty() == false);
		
		Node* back = _tail;
		
		_tail = _tail->prev;
		if (_tail == 0) _head = 0;
		else _tail->next = 0;
		
		delete back;
		
		return *this;
	}

	template <typename _type>
	List<_type>&
	List<_type>::popFront(void) {
		assert(empty() == false);
		
		Node* front = _head;
		
		_head = _head->next;
		if (_head == 0) _tail = 0;
		else _head->prev = 0;
		
		delete front;
		
		return *this;
	}

	template <typename _type>
	List<_type>&
	List<_type>::pushBack(const _type& v)
	{
		Node* newNode = new Node(v);
		
		if (empty())
		{
			_head = _tail = newNode;
			return *this;
		}
		
		newNode->prev = _tail;
		_tail->next = newNode;
		_tail = newNode;
		
		return *this;
	}

	template <typename _type>
	List<_type>&
	List<_type>::pushFront(const _type& v)
	{
		Node* newNode = new Node(v);
		
		if (empty())
		{
			_head = _tail = newNode;
			return *this;
		}
		
		newNode->next = _head;
		_head->prev = newNode;
		_head = newNode;
		
		return *this;
	}
	
	template <typename _type>
	void
	List<_type>::remove(const angle::Enumerator<_type>* e)
	{
		assert(empty() == false);
		assert(e != 0);
		const List<_type>::Enum<_type>* pos = (const List<_type>::Enum<_type>*)e;
		assert(&pos->_c == this);
		
		pos->hasNext(); // used here to refresh the _n member if needed
		
		if (pos->_n == pos->_c._head)
		{
			popFront();
			return;
		}
		
		if (pos->_n == pos->_c._tail)
		{
			popBack();
			return;
		}
		
		pos->_n->prev->next = pos->_n->next;
		pos->_n->next->prev = pos->_n->prev;
		
		delete pos->_n;
	}
	
	template <typename _type>
	angle::Enumerator<_type>*
	List<_type>::revEnumerator(void)
	{
		return new Enum<_type>(*this, true);
	}
	
	/* List::__Enum implementation */
	template <typename _type>
	template <typename _enumerated_type>
	List<_type>::Enum<_enumerated_type>::Enum(const List& l, bool reversed) :
	_c(l),
	_e(false),
	_n(reversed ? l._tail : l._head),
	_reversed(reversed)
	{
		if (_n) _f = true;
	}
	
	template <typename _type>
	template <typename _enumerated_type>
	bool
	List<_type>::Enum<_enumerated_type>::hasNext(void) const
	{
		if (!_n && !_e)
		{
			if (!_reversed && _c._head != 0)
			{
				Node** n = (Node**)&_n;
				*n = _c._head;
				bool* f = (bool*)&_f;
				*f = true;
			} else if (_reversed && _c._tail != 0)
			{
				Node** n = (Node**)&_n;
				*n = _c._tail;
				bool* f = (bool*)&_f;
				*f = true;
			} else
			{
				return false;
			}
		}
		return _f || _n != 0;
	}
	
	template <typename _type>
	template <typename _enumerated_type>
	bool
	List<_type>::Enum<_enumerated_type>::hasPrevious(void) const
	{
		if (!_n && _e && !_reversed) return _c._tail->prev != 0;
		if (!_n && _e && _reversed) return _c._head->next != 0;
		if (!_n) return false;
		return _reversed ? _n->next != 0 : _n->prev != 0;
	}
	
	template <typename _type>
	template <typename _enumerated_type>
	_enumerated_type*
	List<_type>::Enum<_enumerated_type>::next(void)
	{
		if (false == hasNext()) return 0;
		_enumerated_type* v = &_n->value;
		if (_f == true) _f = false;
		
		if (_reversed) _n = _n->prev;
		else _n = _n->next;
		
		if (_n == 0) _e = true;
		return v;
	}
	
	template <typename _type>
	template <typename _enumerated_type>
	_enumerated_type*
	List<_type>::Enum<_enumerated_type>::previous(void)
	{
		if (false == hasPrevious()) return 0;
		
		if (!_n && _e && _reversed) return &_c._head->next->value;
		if (!_n && _e && !_reversed) return &_c._tail->prev->value;
		
		if (_reversed) _n = _n->next;
		else _n = _n->prev;
		
		return &_n->value;
	}
	
	/* List::Node implementation */
	template <typename _type>
	List<_type>::Node::Node(const _type& val) :
	next(0),
	prev(0),
	value(val)
	{ }
	
	template <typename _type>
	List<_type>::Node::Node(const Node& copy) :
	next(copy.next()),
	prev(copy.prev()),
	value(copy.value())
	{ }
	
	template <typename _type>
	List<_type>::Node::Node(const Node& copy, const Node& next, const Node& prev) :
	next(next),
	prev(prev),
	value(copy.val)
	{ }
}

#endif
