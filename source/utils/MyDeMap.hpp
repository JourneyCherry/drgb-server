#pragma once
#include <map>
#include <mutex>
#include <thread>
#include "MyExpected.hpp"

template <typename LKey, typename RKey, typename Value>
class MyDeMap
{
	private:
		using ulock = std::unique_lock<std::mutex>;
		std::mutex mtx;
		std::map<LKey, RKey> lrm;
		std::map<RKey, LKey> rlm;
		std::map<LKey, Value> kvm;
	
	public:
		MyDeMap() = default;
		~MyDeMap() = default;
		bool Insert(LKey lkey, RKey rkey, Value value)
		{
			ulock lk(mtx);

			if(lrm.find(lkey) != lrm.end() || rlm.find(rkey) != rlm.end())
				return false;

			lrm.insert({lkey, rkey});
			rlm.insert({rkey, lkey});
			kvm.insert({lkey, value});

			return true;
		}
		MyExpected<std::pair<RKey, Value>> FindLKey(LKey key)
		{
			ulock lk(mtx);

			if(lrm.find(key) == lrm.end())
				return {};
			
			return {{lrm[key], kvm[key]}};
		}
		MyExpected<std::pair<LKey, Value>> FindRKey(RKey key)
		{
			ulock lk(mtx);

			if(rlm.find(key) == rlm.end())
				return {};

			return {{rlm[key], kvm[rlm[key]]}};
		}
		bool InsertLKeyValue(LKey lkey, Value value)
		{
			ulock lk(mtx);

			if(kvm.find(lkey) == kvm.end())
				return false;

			kvm[lkey] = value;

			return true;
		}
		bool InsertRKeyValue(RKey rkey, Value value)
		{
			ulock lk(mtx);

			if(rlm.find(rkey) == rlm.end())
				return false;
			
			kvm[rlm[rkey]] = value;

			return true;
		}
		MyExpected<std::tuple<LKey, RKey, Value>> EraseLKey(LKey lkey)
		{
			ulock lk(mtx);

			if(lrm.find(lkey) == lrm.end())
				return {};
			
			RKey rkey = lrm[lkey];
			Value value = kvm[lkey];

			lrm.erase(lkey);
			rlm.erase(rkey);
			kvm.erase(lkey);

			return {{lkey, rkey, value}, true};
		}

		MyExpected<std::tuple<LKey, RKey, Value>> EraseRKey(RKey rkey)
		{
			ulock lk(mtx);

			if(rlm.find(rkey) == rlm.end())
				return {};
			
			LKey  lkey = rlm[lkey];
			Value value = kvm[lkey];

			lrm.erase(lkey);
			rlm.erase(rkey);
			kvm.erase(lkey);

			return {{lkey, rkey, value}, true};
		}

		void Clear()
		{
			ulock lk(mtx);

			lrm.clear();
			rlm.clear();
			kvm.clear();
		}
		size_t Size() const
		{
			return lrm.size();
		}
};