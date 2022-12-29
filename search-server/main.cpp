// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

/*
Разместите код остальных тестов здесь
*/
// Добавление документов.
void TestFindAddedDocument() {
    SearchServer server;
    ASSERT_EQUAL(server.GetDocumentCount(), 0);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    ASSERT_EQUAL(server.GetDocumentCount(), 1);
    server.AddDocument(1, "ухоженный пес выразительные глаза", DocumentStatus::ACTUAL, { 1, 2, 3 });
    ASSERT_EQUAL(server.GetDocumentCount(), 2);
    const auto found_docs = server.FindTopDocuments("кот"s);
    ASSERT_EQUAL(found_docs.size(), 1);
    const Document& doc0 = found_docs[0];
    ASSERT_EQUAL(doc0.id, 0);
}

// Поддержка минус-слов.
void TestMinusWords() {
    SearchServer server;
    server.AddDocument(1, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(2, "ухоженный пес выразительные глаза"s, DocumentStatus::ACTUAL, { 1,2,3 });

    {
        const auto found_docs = server.FindTopDocuments("белый -пес"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        ASSERT_EQUAL(found_docs[0].id, 1);
    }

    {
        const auto found_docs = server.FindTopDocuments("ухоженный -кот"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        ASSERT_EQUAL(found_docs[0].id, 2);
    }

}

// Соответствие документов поисковому запросу. 
void TestMatched() {
    SearchServer server;
    server.SetStopWords("и в на"s);
    const int id = 0;
    server.AddDocument(id, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2, 3 });

    {
        const auto [matched_words, status] = server.MatchDocument("белый кот"s, id);
        const vector<string> expected_result = { "белый"s, "кот"s };
        ASSERT_EQUAL_HINT(expected_result, matched_words, "Test 'Matching' is failed"s);
    }

    {
        const auto [matched_words, status] = server.MatchDocument("белый -кот"s, id);
        const vector<string> expected_result = {}; // пустой результат поскольку есть минус-слово 
        ASSERT_EQUAL_HINT(expected_result, matched_words, "Test 'Matching with minus-words' is failed"s);
        ASSERT(matched_words.empty());
    }
}

// Сортировка найденных документов по релевантности. 
void TestSortByRelevance() {
    SearchServer server;
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "ухоженный пес выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::ACTUAL, { 9 });

    {
        const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s);
        int doc_size = found_docs.size();
        for (int i = 0; i < doc_size; i++) {
            ASSERT_HINT(found_docs[i].relevance >= found_docs[i+1].relevance, "Test 'Sorting by relevance' is failed"s);
        }
    }
}

// Вычисление рейтинга документов. 
void TestRatingCalc() {
    SearchServer server;
    const vector<int> ratings = { 5, -12, 2, 1 };
    const int average = (5 -12 + 2 + 1) / 4;
    server.AddDocument(0, "ухоженный пес выразительные глаза"s, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("ухоженный пес"s);
    ASSERT_EQUAL_HINT(found_docs[0].rating, average, "Test 'Rating calc' is failed"s);
}

// Фильтрация результатов поиска с использованием предиката. 
void TestDocumentSearchByPredicate() {
    SearchServer server;
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::IRRELEVANT, { -7, -2, -7 });
    server.AddDocument(2, "ухоженный пес выразительные глаза"s, DocumentStatus::ACTUAL, { -5, -12, 2, 1 });
    const auto found_docs = server.FindTopDocuments("ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return rating > 0; });
    ASSERT_HINT(found_docs[0].id == 0, "Test 'Searching by predicate' is failed"s);
}

// Поиск документов, имеющих заданный статус. 
void TestDocumentSearchByStatus() {
    SearchServer server;
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::IRRELEVANT, { 1, 2, 3 });
    server.AddDocument(2, "ухоженный пес выразительные глаза"s, DocumentStatus::IRRELEVANT, { 1, 2, 3 });
    server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    const auto found_docs = server.FindTopDocuments("ухоженный кот"s, DocumentStatus::IRRELEVANT);
    ASSERT_HINT(found_docs[0].id == 1, "Test 'Searching by status' is failed"s);
    ASSERT_HINT(found_docs[1].id == 2, "Test 'Searching by status' is failed"s);
    ASSERT_HINT(found_docs.size() == 2, "Test 'Searching by status' is failed"s);    
}

// Корректное вычисление релевантности найденных документов. 
void TestCalculateRelevance() {
    SearchServer server;
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(2, "ухоженный пес выразительные глаза"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s); 
    double relevance_query = log((3 * 1.0) / 1) * (2.0 / 4.0) + log((3 * 1.0) / 2) * (1.0 / 4.0);
    ASSERT_HINT(fabs(found_docs[0].relevance - relevance_query) < 1e-6, "Test 'Relevance calculating' is failed"s);
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestFindAddedDocument);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatched);
    RUN_TEST(TestSortByRelevance);
    RUN_TEST(TestRatingCalc);
    RUN_TEST(TestDocumentSearchByPredicate);
    RUN_TEST(TestDocumentSearchByStatus);
    RUN_TEST(TestCalculateRelevance);
    // Не забудьте вызывать остальные тесты здесь
}

// --------- Окончание модульных тестов поисковой системы -----------
