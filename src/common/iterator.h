
#pragma once

#include "common/refcountedobj.h"
#include <vector>
#include <map>

template <class T>
class BaseIterator : public RefCountedObj {
public:
	virtual bool next(T *value) = 0;
};


template <class T>
class Iterator {
public:
	Iterator(BaseIterator<T> *iter) {
		iterator = iter;
		eof = !iter->next(&value);
	}
	
	Iterator<T> begin() { return *this; }
	Iterator<T> end() { return *this; }
	
	bool operator !=(const Iterator<T> &other) {
		return !eof;
	}
	
	Iterator<T> &operator ++() {
		eof = !iterator->next(&value);
		return *this;
	}
	
	T operator *() {
		return value;
	}
	
private:
	Ref<BaseIterator<T>> iterator;
	
	bool eof;
	T value;
};


template <class T>
class VectorIterator : public BaseIterator<T> {
public:
	VectorIterator(const std::vector<T> &obj) {
		begin = obj.begin();
		end = obj.end();
	}
	
	bool next(T *value) {
		if (begin == end) {
			return false;
		}
		
		*value = *begin++;
		return true;
	}
	
private:
	typename std::vector<T>::const_iterator begin;
	typename std::vector<T>::const_iterator end;
};


template <class K, class V>
class MapIterator : public BaseIterator<std::pair<K, V>> {
public:
	MapIterator(const std::map<K, V> &obj) {
		begin = obj.begin();
		end = obj.end();
	}
	
	bool next(std::pair<K, V> *value) {
		if (begin == end) {
			return false;
		}
		
		*value = *begin++;
		return true;
	}
	
private:
	typename std::map<K, V>::const_iterator begin;
	typename std::map<K, V>::const_iterator end;
};


template <class K, class V>
class MapKeyIterator : public BaseIterator<V> {
public:
	MapKeyIterator(const std::map<K, V> &obj) : iterator(obj) {}
	
	bool next(V *value) {
		std::pair<K, V> pair;
		if (iterator.next(&pair)) {
			*value = pair.first;
			return true;
		}
		return false;
	}
	
private:
	MapIterator<K, V> iterator;
};


template <class K, class V>
class MapValueIterator : public BaseIterator<V> {
public:
	MapValueIterator(const std::map<K, V> &obj) : iterator(obj) {}
	
	bool next(V *value) {
		std::pair<K, V> pair;
		if (iterator.next(&pair)) {
			*value = pair.second;
			return true;
		}
		return false;
	}
	
private:
	MapIterator<K, V> iterator;
};
