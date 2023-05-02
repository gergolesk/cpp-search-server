#pragma once

#include "search_server.h"
#include "log_duration.h"

#include <string>
#include <iostream>



void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,
                 const std::vector<int>& ratings);

void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query) ;

void MatchDocuments(const SearchServer& search_server, const std::string& query);