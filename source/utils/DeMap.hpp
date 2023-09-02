#pragma once
#include <map>
#include <mutex>
#include <thread>
#include "Expected.hpp"

namespace mylib{
namespace utils{

/**
 * @brief dual-key map. it's identical with std::map<std::pair<LKey, RKey>, Value>.
 * LKey and RKey can be same but same key for each is not acceptable.
 * ---
 * For example, {a, a} can be accepted.
 * In addition, if there is a key {a, b}, {b, a} is acceptable, but {a, c} or {c, b} is not acceptable.
 * ---
 * 
 * @tparam LKey First Key Type
 * @tparam RKey Second Key Type
 * @tparam Value Value Type
 */
template <typename LKey, typename RKey, typename Value>
class DeMap
{
	private:
		using ulock = std::unique_lock<std::mutex>;
		std::mutex mtx;
		std::map<LKey, RKey> lrm;
		std::map<RKey, LKey> rlm;
		std::map<LKey, Value> kvm;
	
	public:
		DeMap() = default;
		~DeMap() = default;
		/**
		 * @brief Insert data.(Thread-Safe)
		 * 
		 * @param lkey First Key
		 * @param rkey Second Key
		 * @param value Value
		 * @return true if successed.
		 * @return false if failed with duplication.
		 */
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
		/**
		 * @brief Find value with First Key.(Thread-Safe)
		 * 
		 * @param key First Key
		 * @return Expected<std::pair<RKey, Value>> Expected result with Second key and value.
		 */
		Expected<std::pair<RKey, Value>> FindLKey(LKey key)
		{
			ulock lk(mtx);

			if(lrm.find(key) == lrm.end())
				return {};
			
			return {{lrm[key], kvm[key]}};
		}
		/**
		 * @brief Find value with Second Key.(Thread-Safe)
		 * 
		 * @param key First Key
		 * @return Expected<std::pair<LKey, Value>> Expected result with First key and value.
		 */
		Expected<std::pair<LKey, Value>> FindRKey(RKey key)
		{
			ulock lk(mtx);

			if(rlm.find(key) == rlm.end())
				return {};

			return {{rlm[key], kvm[rlm[key]]}};
		}
		/**
		 * @brief Change Value with First Key.(Thread-Safe)
		 * 
		 * @param lkey First Key
		 * @param value Value to Change
		 * @return true if there is the key.
		 * @return false if there is no key.
		 */
		bool InsertLKeyValue(LKey lkey, Value value)
		{
			ulock lk(mtx);

			if(kvm.find(lkey) == kvm.end())
				return false;

			kvm[lkey] = value;

			return true;
		}
		/**
		 * @brief Change Value with Second Key.(Thread-Safe)
		 * 
		 * @param rkey Second Key
		 * @param value Value to Change
		 * @return true if there is the key.
		 * @return false if there is no key.
		 */
		bool InsertRKeyValue(RKey rkey, Value value)
		{
			ulock lk(mtx);

			if(rlm.find(rkey) == rlm.end())
				return false;
			
			kvm[rlm[rkey]] = value;

			return true;
		}
		/**
		 * @brief Erase data with First Key.(Thread-Safe)
		 * 
		 * @param lkey First Key
		 * @return Expected<std::tuple<LKey, RKey, Value>> Expected result of the data.
		 */
		Expected<std::tuple<LKey, RKey, Value>> EraseLKey(LKey lkey)
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
		/**
		 * @brief Erase data with Second Key.(Thread-Safe)
		 * 
		 * @param rkey Second Key
		 * @return Expected<std::tuple<LKey, RKey, Value>> Expected result of the data.
		 */
		Expected<std::tuple<LKey, RKey, Value>> EraseRKey(RKey rkey)
		{
			ulock lk(mtx);

			if(rlm.find(rkey) == rlm.end())
				return {};
			
			LKey  lkey = rlm[rkey];
			Value value = kvm[lkey];

			lrm.erase(lkey);
			rlm.erase(rkey);
			kvm.erase(lkey);

			return {{lkey, rkey, value}, true};
		}
		/**
		 * @brief Remove all data.(Thread-Safe)
		 * 
		 */
		void Clear()
		{
			ulock lk(mtx);

			lrm.clear();
			rlm.clear();
			kvm.clear();
		}
		/**
		 * @brief Get size of all data.
		 * 
		 * @return size_t 
		 */
		size_t Size() const
		{
			return lrm.size();
		}
		/**
		 * @brief Get all data in STL Vector Container.(Thread-Safe)
		 * 
		 * @return std::vector<std::tuple<LKey, RKey, Value>> all data at called time.
		 */
		std::vector<std::tuple<LKey, RKey, Value>> GetAll()
		{
			ulock lk(mtx);

			std::vector<std::tuple<LKey, RKey, Value>> result;
			result.reserve(lrm.size());
			for(auto &[lkey, value] : kvm)
			{
				auto rkey = lrm[lkey];
				result.push_back({lkey, rkey, value});
			}

			return result;
		}
};

}
}