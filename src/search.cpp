#include "headers/search.h"
#include "headers/message.h"
#include "headers/datetime.h"
#include <algorithm>
#include <sstream>
#include <vector>

static int levenshteinDistance(const std::string& a, const std::string& b) {
    std::vector<std::vector<int>> matrix(a.size() + 1, std::vector<int>(b.size() + 1));

    for (size_t i = 0; i <= a.size(); ++i) {
        matrix[i][0] = static_cast<int>(i);
    }
    for (size_t j = 0; j <= b.size(); ++j) {
        matrix[0][j] = static_cast<int>(j);
    }

    for (size_t i = 1; i <= a.size(); ++i) {
        for (size_t j = 1; j <= b.size(); ++j) {
            if (a[i - 1] == b[j - 1]) {
                matrix[i][j] = matrix[i - 1][j - 1];
            } else {
                matrix[i][j] = 1 + std::min({matrix[i - 1][j], matrix[i][j - 1], matrix[i - 1][j - 1]});
            }
        }
    }

    return matrix[a.size()][b.size()];
}

static double similarity(const std::string& a, const std::string& b) {
    size_t maxLength = std::max(a.size(), b.size());
    if (maxLength == 0) {
        return 1.0;
    }

    int distance = levenshteinDistance(a, b);
    return 1.0 - static_cast<double>(distance) / static_cast<double>(maxLength);
}

static std::vector<std::string> splitWords(const std::string& text) {
    std::istringstream stream(text);
    std::vector<std::string> words;
    std::string word;

    while (stream >> word) {
        words.push_back(word);
    }

    return words;
}

// Для каждого слова запроса берём лучшее совпадение среди слов поля,
// итог — среднее этих лучших совпадений. Так многословный запрос
// не обязан совпадать целиком и подряд, а поле может быть длиннее запроса.
static double fieldScore(const std::vector<std::string>& queryWords, const std::string& field) {
    std::vector<std::string> fieldWords = splitWords(field);

    if (queryWords.empty() || fieldWords.empty()) {
        return 0.0;
    }

    double total = 0.0;
    for (const std::string& queryWord : queryWords) {
        double best = 0.0;
        for (const std::string& fieldWord : fieldWords) {
            best = std::max(best, similarity(queryWord, fieldWord));
        }
        total += best;
    }

    return total / static_cast<double>(queryWords.size());
}

std::vector<Message> searchMessages(const std::string& query, const std::vector<Message>& messages) {
    const double threshold = 0.7;

    std::vector<std::string> queryWords = splitWords(query);
    std::vector<std::pair<double, size_t>> scored;

    for (size_t i = 0; i < messages.size(); ++i) {
        double authorScore = fieldScore(queryWords, messages[i].author);
        double textScore = fieldScore(queryWords, messages[i].text);
        double score = std::max(authorScore, textScore);

        if (score >= threshold) {
            scored.push_back({score, i});
        }
    }

    std::sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) {
        return a.first > b.first;
    });

    std::vector<Message> results;
    size_t count = std::min(scored.size(), static_cast<size_t>(5));
    for (size_t i = 0; i < count; ++i) {
        results.push_back(messages[scored[i].second]);
    }

    return results;
}
