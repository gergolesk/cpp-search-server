#pragma once
#include <iostream>

enum DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

struct Document {
    Document();
    Document(int id_doc, double relevance_doc, int rating_doc);

    int id;
    double relevance;
    int rating;
};

std::ostream& operator<<(std::ostream& out, const Document& doc);