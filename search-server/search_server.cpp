#include "search_server.h"
#include "log_duration.h"

#include<iostream>

using namespace std;

SearchServer::SearchServer(const string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

SearchServer::SearchServer(const string_view stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

void SearchServer::AddDocument(int document_id, const string_view document, DocumentStatus status, const vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument(" Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    
    for (const string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        id_to_words_freqs_[document_id][word] += inv_word_count;
        
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}

vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy, const string_view raw_query, DocumentStatus status) const {
    std::execution::parallel_policy policy;
    return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy, const string_view raw_query) const {
    std::execution::parallel_policy policy;
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}


vector<Document> SearchServer::FindTopDocuments(std::execution::sequenced_policy, const string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

vector<Document> SearchServer::FindTopDocuments(std::execution::sequenced_policy, const string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}


vector<Document> SearchServer::FindTopDocuments(const string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

vector<Document> SearchServer::FindTopDocuments(const string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

std::tuple<std::vector<std::string_view>, DocumentStatus>
SearchServer::MatchDocument(
              const std::execution::parallel_policy& policy,
              const std::string_view raw_query,
              int document_id) const {
    assert(documents_.count(document_id) > 0);

    const auto& word_freqs = id_to_words_freqs_.at(document_id);
    static const auto query = ParseQuery( raw_query);

    if (std::any_of(policy, query.minus_words.begin(), query.minus_words.end(), [&word_freqs](const std::string word) {
        return word_freqs.count(word) > 0;
        
    })) {
        return { {}, documents_.at(document_id).status };
    }


    static std::vector<std::string_view> matched_words;
    
    matched_words.clear();
    matched_words.reserve(query.plus_words.size());
    std::copy_if( query.plus_words.begin(),
                 query.plus_words.end(), back_inserter(matched_words),
                 [&word_freqs](const std::string word) {
                     return word_freqs.count(word) > 0 ;
                 });
   
    std::sort( matched_words.begin(), matched_words.end());
    matched_words.erase(std::unique( matched_words.begin(), matched_words.end()), matched_words.end());

    return { matched_words, documents_.at(document_id).status };
}

std::tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy& policy, const string_view raw_query, int document_id) const {
    
    if (documents_.count(document_id) == 0) {
        throw std::out_of_range("id out of range");
    }
    
    static const auto query = ParseQuery(raw_query);

    static vector<string_view> matched_words;
    matched_words.clear();
    for (const string word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const string word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    
    return {matched_words, documents_.at(document_id).status};
    
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const string_view raw_query, int document_id) const {
    if (documents_.count(document_id) == 0) {
        throw std::out_of_range("id out of range");
    }
    static const auto query = ParseQuery(raw_query);
    
    static vector<string_view> matched_words;
    matched_words.clear();
    
    for (const string& word : query.plus_words) {
        
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            
            matched_words.push_back(word);
            
        }
    }
    for (const string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    
    return {matched_words, documents_.at(document_id).status};
}

bool SearchServer::IsStopWord(const string& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const string& word) {
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

vector<string> SearchServer::SplitIntoWordsNoStop(const string_view text) const {
    vector<string> words;
    for (const auto word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + word + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const string& text) const {
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    string word = text;
    bool is_minus = false;
    if (*word.begin() == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || *word.begin() == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + text + " is invalid");
    }

    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(const string_view text) const {
    
    Query result;
    
    for (const string& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    
    std::sort( result.plus_words.begin(), result.plus_words.end());
    result.plus_words.erase(std::unique( result.plus_words.begin(), result.plus_words.end()), result.plus_words.end());
    
    std::sort( result.minus_words.begin(), result.minus_words.end());
    result.minus_words.erase(std::unique( result.minus_words.begin(), result.minus_words.end()), result.minus_words.end());
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

std::set<int>::const_iterator SearchServer::begin() const {
    
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static map<string_view, double> empty_dictionary;
    static map<string_view, double> dictionary;
    if(documents_.count(document_id) != 0 ) {
        for(auto& [word, freq] : id_to_words_freqs_.at(document_id)) {
            dictionary[word] = freq;
        }
        return dictionary;
    } else {
        return empty_dictionary;
    }
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy& policy, int document_id) {
    if (documents_.count(document_id) == 0) {
        return;
    }
    
    
    std::map<std::string, double> words_freqs(std::move(id_to_words_freqs_.at(document_id))) ;
    
    documents_.erase(document_id);

    document_ids_.erase(find(policy, document_ids_.begin(), document_ids_.end(), document_id));
    
    std::vector<const std::basic_string<char>*> words(words_freqs.size());
   
    std::transform(
        policy,
        std::make_move_iterator(words_freqs.begin()),
        std::make_move_iterator(words_freqs.end()),
        std::make_move_iterator(words.begin()),
        []( auto& a) {
             
            return const_cast<std::basic_string<char>*>(&(a.first));
        }
    );
   
    
    std::for_each(policy, words.begin(), words.end(), [this, document_id](auto a) {
        word_to_document_freqs_[*a].erase(document_id);
       
    });
    
    
    id_to_words_freqs_.erase(document_id);

}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy& policy, int document_id) {
    if (documents_.count(document_id) == 0) {
        return;
    }
    
    documents_.erase(document_id);

    document_ids_.erase(find(document_ids_.begin(), document_ids_.end(), document_id));

    for(auto& [word, _] : id_to_words_freqs_[document_id] ) {
        word_to_document_freqs_[word].erase(document_id);
    }

    id_to_words_freqs_.erase(document_id);

}

void SearchServer::RemoveDocument(int document_id) {
    const std::execution::sequenced_policy policy;
    RemoveDocument(policy, document_id);
}