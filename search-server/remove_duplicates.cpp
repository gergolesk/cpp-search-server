#include <iostream>
#include "remove_duplicates.h"

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
    set<set<string>> docs;
    vector<int> found_duplicates;

    for (int id : search_server) {
        auto& freqs = search_server.GetWordFrequencies(id);
        set<string> words_in_document;

        transform(freqs.begin(), freqs.end(), inserter(words_in_document, words_in_document.begin()),
            [](auto p) {
                return p.first;
            });

        if (docs.count(words_in_document) > 0) {
            cout << "Found duplicate document id " << id << endl;
            found_duplicates.push_back(id);
        }
        else {
            docs.insert(words_in_document);
        }
    }

    for (int id : found_duplicates) {
        search_server.RemoveDocument(id);
    }
}
