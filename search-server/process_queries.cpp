#include "process_queries.h"


using namespace std;

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> d_vtr(queries.size());
    std::transform(
        std::execution::par,
        std::make_move_iterator(queries.begin()), std::make_move_iterator(queries.end()),
        std::make_move_iterator(d_vtr.begin()),
        [&search_server](const string& query) {
            return search_server.FindTopDocuments(query);
        }
    );
    return d_vtr;
}

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    vector<Document> result;
    for (const auto& doc : ProcessQueries(search_server, queries)) {
        result.insert(result.end(), doc.begin(),doc.end());
    }
        
        
   return result;
}