#include "document.h"

using namespace std;

Document::Document() : id(0),
relevance(0),
rating(0) {
}

Document::Document(int id, double relevance, int rating) : id(id),
relevance(relevance),
rating(rating) {
}

ostream& operator<<(ostream& out, const Document& doc) {
    out << "{ document_id = "s << doc.id
        << ", relevance = "s << doc.relevance
        << ", rating = "s << doc.rating << " }"s;
    return out;
}