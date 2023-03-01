#pragma once
#include "document.h"
#include "search_server.h"

void PrintDocument(const Document& document);

void AddDocument(SearchServer& search_server, int document_id, std::string document, DocumentStatus status,
    const std::vector<int>& ratings);