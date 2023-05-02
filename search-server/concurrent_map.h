#pragma once

#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <mutex>
#include <execution>

using namespace std;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
    
    Access(mutex& mut, Value& value)
        :localm(mut), ref_to_value(value){
        //localm.unlock();

    }
    ~Access() {
        localm.unlock();
    }
        mutex& localm;
        Value& ref_to_value;
        
    };

    explicit ConcurrentMap(size_t bucket_count)
        :muts(bucket_count), maps(bucket_count) {
            
        }

    Access operator[](const Key& key){
        
        atomic_uint64_t ks = key % muts.size();
        muts[ks].lock();
        
        return {muts[ks], maps[ks][key]};
    }
    
    map<Key, Value> BuildOrdinaryMap() {
    
        map<Key, Value> ret;
        
        for(atomic_size_t i = 0; i < maps.size(); i.fetch_add(1)) {
            
            for(auto& a : maps[i]) {
                lock_guard lggt(muts[i]);
                ret[a.first] = a.second;

            }
        }
        
        return ret;
    }
    

private:
    vector<mutex> muts;
    vector<map<Key, Value>> maps;
};