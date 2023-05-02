#pragma once

#include "document.h"
#include "paginator.h"
#include "read_input_functions.h"
#include "request_queue.h"
#include "search_server.h"
#include "string_processing.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    
    template<typename Query>
    int SetTime(const Query& prev);
    
 
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status) ;
    std::vector<Document> AddFindRequest(const std::string& raw_query) ;

    int GetNoResultRequests() const ;
private:
    struct QueryResult {
        int time;
        bool empty_request;
        // определите, что должно быть в структуре
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    // возможно, здесь вам понадобится что-то ещё
    const SearchServer& search_server_;
};


template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    QueryResult local;
    std::vector<Document> find_local = search_server_.FindTopDocuments(raw_query, document_predicate);
    
    local.empty_request = find_local.empty();
    
    
    if (requests_.size() != 0) {
        
        local.time = SetTime(requests_.back());
        if (requests_.size() == min_in_day_){
            requests_.pop_front();
        }
        requests_.push_back(local);
    } else {
        local.time = 1;
        requests_.push_back(local);
    }
    return find_local;
}


template<typename Query>
int RequestQueue::SetTime(const Query& prev) {
    if (prev.time < min_in_day_) {
        return 1 + prev.time;
    } else {
        return 1;
    }
}