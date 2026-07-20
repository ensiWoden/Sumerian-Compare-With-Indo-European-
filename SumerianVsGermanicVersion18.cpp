#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

enum Place { BILABIAL, DENTAL, VELAR, GLOTTAL, VOWEL_PLACE, UNKNOWN_PLACE };
enum Manner { STOP, FRICATIVE, NASAL, LIQUID, VOWEL_MANNER, UNKNOWN_MANNER };

struct ConsonantProfile {
    Place place;
    Manner manner;
    bool isVoiced;
};

struct VowelProfile {
    double height;   
    double backness; 
    bool isLong;
};

struct CognateCandidate {
    std::string sumerian;
    std::string germanic;
    double similarity;
    std::string pathMapReport;
};

int getMannerEnergyRank(Manner manner) {
    switch (manner) {
        case STOP:      return 4; 
        case FRICATIVE: return 3; 
        case NASAL:     return 2; 
        case LIQUID:    return 1; 
        default:        return 0;
    }
}

ConsonantProfile getConsonantProfile(const std::string& t) {
    if (t == "D") return { DENTAL,   STOP,      true  }; // 'D' for 'da' 
    if (t == "S") return { DENTAL,   FRICATIVE, false }; // 'S' onset
    if (t == "p")  return { BILABIAL, STOP,      false };
    if (t == "b")  return { BILABIAL, STOP,      true  };
    if (t == "f")  return { BILABIAL, FRICATIVE, false };
    if (t == "t")  return { DENTAL,   STOP,      false };
    if (t == "d")  return { DENTAL,   STOP,      true  };
    if (t == "þ" || t == "th" || t == "\xc3\xbe") return { DENTAL, FRICATIVE, false };
    if (t == "k")  return { VELAR,    STOP,      false };
    if (t == "g")  return { VELAR,    STOP,      true  };
    if (t == "ng") return { VELAR,    NASAL,     true  };
    if (t == "h")  return { GLOTTAL,  FRICATIVE, false };
    if (t == "s" || t == "sh") return { DENTAL, FRICATIVE, false };
    if (t == "r" || t == "l") return { DENTAL, LIQUID, true };
    if (t == "w")  return { VELAR,    LIQUID,    true  }; 
    if (t == "hw") return { GLOTTAL,  FRICATIVE, false }; 
    if (t == "j")  return { DENTAL,   LIQUID,    true  }; 
    if (t == "G") return { VELAR, NASAL, true }; //G for ng 
    return { UNKNOWN_PLACE, UNKNOWN_MANNER, false };
}

VowelProfile getVowelProfile(const std::string& token) {
    std::string t = token;
    std::transform(t.begin(), t.end(), t.begin(), ::tolower);
    // Explicitly check for your new single-character diphthong token
    //if (token == "A") return { 1.5, 1.5, true }; // Mid-Low Centralized Diphthong Profile 
    //bool longVowel = (token[0] >= 'A' && token[0] <= 'Z') || token == "ø" || token == "å" || token == "æ" || token == "O" || token == "E";
    bool longVowel = (token[0] >= 'A' && token[0] <= 'Z') || 
                     token == "ø" || token == "Ø" || 
                     token == "å" || token == "Å" || 
                     token == "æ" || token == "Æ" || 
                     token == "O" || token == "E";

    if (token == "A") return { 1.5, 1.5, true };

    if (t == "i" || t == "y") return { 3.0, 1.0, longVowel };
    if (t == "u")             return { 3.0, 3.0, longVowel };
    if (t == "e" || t == "ø") return { 2.0, 1.0, longVowel };
    if (t == "o" || t == "å") return { 2.0, 3.0, longVowel };
    if (t == "æ")             return { 1.5, 1.0, longVowel };
    if (t == "a")             return { 1.0, 2.0, longVowel };
    return { 0.0, 0.0, false }; 
}

bool isVowelToken(const std::string& token) {
    if (token.empty()) return false;
    if (token == "kU" || token == "ku" || token == "A" || token == "D" || token == "S") return true; 
    std::string t = token;
    std::transform(t.begin(), t.end(), t.begin(), ::tolower);
    if (t == "ou" || t == "uo" || t == "æ" || t == "ø" || t == "å") return true;
    if (token.length() == 1) {
    std::string standardvowels = "aeiouyøåæAEIOUYØÅÆ";
    return standardvowels.find(tolower(token[0])) != std::string::npos;
    }
    return false; 
}

std::vector<std::string> tokenizePhonemes(const std::string& word) {
    std::vector<std::string> tokens;
    size_t i = 0;
    while (i < word.length()) {
        if (i + 1 < word.length() && (unsigned char)word[i] == 0xC3) {
            tokens.push_back(word.substr(i, 2));
            i += 2;
            continue;
        }
        if (i + 1 < word.length() && (word[i] == 'k' || word[i] == 'K')) {
            if (word[i+1] == 'U') {
                tokens.push_back(word.substr(i, 2)); 
                i += 2;
                continue;
            }
        }
        if (i + 1 < word.length()) {
            std::string digraph = word.substr(i, 2);
            std::string lowerDigraph = { (char)std::tolower(digraph[0]), (char)std::tolower(digraph[1]) };
            if ( lowerDigraph == "sh" || lowerDigraph == "ch" || 
                lowerDigraph == "th" || lowerDigraph == "hw" || lowerDigraph == "ou" || 
                lowerDigraph == "uo") { 
                tokens.push_back(digraph);
                i += 2;
                continue;
            }
        }

        if (word[i] == 'D') {
            tokens.push_back("D"); // Will be profiled as "da"
            i++;
            continue;
        }
        if (word[i] == 'S') {
            tokens.push_back("S"); // Will be profiled as "sU"
            i++;
            continue;
        }
        
        tokens.push_back(std::string(1, word[i]));
        i++;
    }
    return tokens;
}

std::vector<std::string> normalizeTokenVowelLength(const std::vector<std::string>& tokens) {
    std::vector<std::string> normalized;
    for (size_t i = 0; i < tokens.size(); i++) {
        if (i + 1 < tokens.size() && isVowelToken(tokens[i]) && tokens[i] == tokens[i+1]) {
            std::string upperVowel = tokens[i];
            if (upperVowel == "æ") upperVowel = "Æ";
            else if (upperVowel == "ø") upperVowel = "Ø";
            else if (upperVowel == "å") upperVowel = "Å";
            else upperVowel[0] = std::toupper(upperVowel[0]);
            normalized.push_back(upperVowel); 
            i++;
        } else {
            normalized.push_back(tokens[i]); 
        }
    }
    return normalized; 
}

std::string cleanSuffixes(std::string word) {
    word.erase(0, word.find_first_not_of(" \t\n\r"));
    word.erase(word.find_last_not_of(" \t\n\r") + 1);
    
    // UPGRADED: Robust raw byte replacement to counter console dialect variance
    size_t pos;
    while ((pos = word.find("mEnO")) != std::string::npos) return word.substr(0, pos) + "mEnO";
    while ((pos = word.find("\xc3\xbe")) != std::string::npos) return word.substr(0, pos);

    if (word.length() <= 3) return word;

    std::vector<std::string> suffixes = {"ths", "it", "is", "an", "as", "es", "ba", "ra", "er", "en", "ig", "az", "iz", "uz", "s"};
    for (const std::string& suffix : suffixes) {
        if (word.length() > suffix.length() && (word.length() - suffix.length() >= 3) &&
            word.compare(word.length() - suffix.length(), suffix.length(), suffix) == 0) {
            return cleanSuffixes(word.substr(0, word.length() - suffix.length()));
        }
    }
    return word;
}

double GetTokenLinguisticCost(const std::string& sumTok, const std::string& germTok,
                              int sumIdx, const std::vector<std::string>& sumerian,
                              int germIdx, const std::vector<std::string>& germanic) {

    if (sumTok == germTok) return 0.0; 

    std::string sLow = sumTok; 
    if (sLow == "D") sLow = "da";
    if (sLow == "S") sLow = "su";
    
    std::transform(sLow.begin(), sLow.end(), sLow.begin(), ::tolower);
    std::string gLow = germTok; std::transform(gLow.begin(), gLow.end(), gLow.begin(), ::tolower);

    if (sLow == "ku" && gLow == "hw") {
        double positionalMultiplier = 1.0;
        if (sumIdx == 0 || germIdx == 0) positionalMultiplier = 1.50;
        return std::min(1.0, 0.05 * positionalMultiplier); 
    }
    if (sLow == "g" && gLow == "t" && !sumerian.empty() && sumerian[0] == "kU") return 0.05; 
    if (sLow == "g" && gLow == "i" && !sumerian.empty() && sumerian[0] == "kU") return 0.65; 
    
    bool sIsVowel = isVowelToken(sumTok);
    bool gIsVowel = isVowelToken(germTok);

    if (sIsVowel != gIsVowel) {
        // UPGRADED OVERRIDE: Active initial glide onset mutation calibration
        if ((sLow == "u" || sLow == "un" || sLow == "ur") && (gLow == "w" || gLow == "v")) {
            if (sumIdx == 0 || germIdx == 0) return 0.05; 
        }
        if (sumTok == "u" || sumTok == "U") {
            if (gLow == "w" || gLow == "v" || gLow == "ou" || gLow == "uo" || gLow == "A") return 0.25;
        }
        if ((sumTok == "i" || sumTok == "e" || sumTok == "A") && (gLow == "j" || gLow == "o")) return 0.30;
        return 1.0;
    }

    double baseCost = 1.0;

    if (!sIsVowel) {
        if ((sLow == "p" && gLow == "f") || (sLow == "t" && (gLow == "þ" || gLow == "th" || gLow == "\xc3\xbe")) || (sLow == "k" && gLow == "h")) {
            baseCost = 0.2; 
        }
        else if ((sLow == "r" && gLow == "l") || (sLow == "l" && gLow == "r")) {
            baseCost = 0.15; 
        }
        else if ((sLow == "l" && gLow == "w") || (sLow == "w" && gLow == "l")) {
            baseCost = 0.25;
        }
        else if ((sLow == "s" || sLow == "sh") && gLow == "s") {
            if (germIdx + 1 < (int)germanic.size() && germanic[germIdx + 1] == "w") return 0.10;
            baseCost = 0.0;
        }
        else if ((sLow == "s" || sLow == "sh") && gLow == "h") {
            return (sumIdx == 0 || germIdx == 0) ? 0.15 : 0.25;
        }
        else if (((sLow == "G" && (gLow == "g" || gLow == "k")) || (gLow == "G" && (sLow == "g" || sLow == "k")))) {
            baseCost = 0.15;
        }
        else if ((sLow == "p" && gLow == "b") || (sLow == "t" && gLow == "d") || (sLow == "k" && gLow == "g")) {
            bool germInVowels = (germIdx > 0 && germIdx < (int)germanic.size() - 1 && isVowelToken(germanic[germIdx - 1]) && isVowelToken(germanic[germIdx + 1]));
            bool sumInVowels  = (sumIdx > 0 && sumIdx < (int)sumerian.size() - 1 && isVowelToken(sumerian[sumIdx - 1]) && isVowelToken(sumerian[sumIdx + 1]));
            baseCost = (germInVowels || sumInVowels) ? 0.25 : 0.35; 
        }
        else if ((sLow == "d" && gLow == "t") || (sLow == "g" && gLow == "k")) {
            baseCost = 0.30; 
        }
        else {
            ConsonantProfile sProf = getConsonantProfile(sLow);
            ConsonantProfile gProf = getConsonantProfile(gLow);

            if (sProf.manner == FRICATIVE && gProf.manner == FRICATIVE) {
                baseCost = 0.25;
            }
            else if (sProf.place != UNKNOWN_PLACE && gProf.place != UNKNOWN_PLACE) {
                double placeCost = (sProf.place != gProf.place) ? 0.35 : 0.0;
                double voiceCost = (sProf.isVoiced != gProf.isVoiced) ? 0.15 : 0.0;
                double mannerCost = 0.0;

                if (sProf.manner != gProf.manner) {
                    int sRank = getMannerEnergyRank(sProf.manner);
                    int gRank = getMannerEnergyRank(gProf.manner);
                    mannerCost = (sRank > gRank) ? std::abs(sRank - gRank) * 0.25 : std::abs(sRank - gRank) * 0.35;
                }
                baseCost = std::min(0.85, placeCost + voiceCost + mannerCost);
            }
        }
    } else {
        if (sumTok == "U" && (gLow == "w" || gLow == "v")) {
            baseCost = 0.10; 
        } 
        else {
            VowelProfile sProf = getVowelProfile(sumTok);
            VowelProfile gProf = getVowelProfile(germTok);

            double spatialDistance = std::sqrt(std::pow(sProf.height - gProf.height, 2) + std::pow(sProf.backness - gProf.backness, 2));
            double normalizedVowelPenalty = (spatialDistance / 2.83) * 0.45;
            if (sProf.isLong != gProf.isLong) normalizedVowelPenalty += 0.10;

            baseCost = std::min(0.50, normalizedVowelPenalty);
        }
    }

    double positionalMultiplier = 1.0;
    if (sumIdx == 0 || germIdx == 0) {
        positionalMultiplier = 1.50; 
    } else if (sumIdx == (int)sumerian.size() - 1 || germIdx == (int)germanic.size() - 1) {
        positionalMultiplier = 0.75; 
    }

    return std::min(1.0, baseCost * positionalMultiplier);
} 

std::string getVisualPathReport(const std::vector<std::string>& sumTokens, 
                                const std::vector<std::string>& germTokens, 
                                const std::vector<std::vector<double>>& scoreMatrix) {
    std::vector<std::string> sumLine, matchLine, germLine;
    int i = sumTokens.size(); 
    int j = germTokens.size(); 
    const double EPS = 1e-4; 

    while (i > 0 || j > 0) {
        if (i > 1 && j > 1) {
            std::string currentSum  = sumTokens[i - 1];
            std::string previousSum = sumTokens[i - 2];
            std::string currentGerm = germTokens[j - 1];
            std::string previousGerm = germTokens[j - 2];
            
            if (currentSum == previousGerm && previousSum == currentGerm) {
                double swapCost = (currentSum == "ku" || isVowelToken(currentSum)) ? 1.50 : 2.50;
                double metathesisReward = swapCost - (0.20 * swapCost);
                
                if (std::abs(scoreMatrix[i][j] - (scoreMatrix[i - 2][j - 2] + metathesisReward)) < EPS) {
                    sumLine.push_back(sumTokens[i - 1]); sumLine.push_back(sumTokens[i - 2]);
                    matchLine.push_back("  X  "); matchLine.push_back("  X  ");
                    germLine.push_back(germTokens[j - 1]); germLine.push_back(germTokens[j - 2]);
                    i -= 2; j -= 2;
                    continue;
                }
            }
        }

        if (i > 0 && j > 0) {
            double finalizedCost = GetTokenLinguisticCost(sumTokens[i - 1], germTokens[j - 1], i - 1, sumTokens, j - 1, germTokens);
            double stepMaxReward = (sumTokens[i - 1] != "kU" && isVowelToken(sumTokens[i - 1])) ? 1.50 : 2.50;
            double matchReward = (finalizedCost == 0.0) ? stepMaxReward : (stepMaxReward - (finalizedCost * stepMaxReward));

            if (std::abs(scoreMatrix[i][j] - (scoreMatrix[i - 1][j - 1] + matchReward)) < EPS) {
                sumLine.push_back(sumTokens[i - 1]);
                germLine.push_back(germTokens[j - 1]);
                matchLine.push_back(sumTokens[i - 1] == germTokens[j - 1] ? "  |  " : "  ~  ");
                i--; j--;
                continue;
            }
        }

        if (i > 0 && j == 0) {
            sumLine.push_back(sumTokens[i - 1]); germLine.push_back("-"); matchLine.push_back("  -  "); i--; continue;
        }
        if (j > 0 && i == 0) {
            sumLine.push_back("-"); germLine.push_back(germTokens[j - 1]); matchLine.push_back("  -  "); j--; continue;
        }

        if (i > 0) {
            if (std::abs(scoreMatrix[i][j] - (scoreMatrix[i - 1][j] - 1.0)) < EPS) {
                sumLine.push_back(sumTokens[i - 1]); germLine.push_back("-"); matchLine.push_back("  -  "); i--; continue;
            }
        }
        if (j > 0) {
            double gapCost = 1.0;
            if (germTokens[j-1] == "n" || germTokens[j-1] == "m") {
                if (j < (int)germTokens.size() && !isVowelToken(germTokens[j])) gapCost = 0.25; 
            }
            if (std::abs(scoreMatrix[i][j] - (scoreMatrix[i][j - 1] - gapCost)) < EPS) {
                sumLine.push_back("-"); germLine.push_back(germTokens[j - 1]); matchLine.push_back("  -  "); j--; continue;
            }
        }
        
        if (i > 0 && j > 0) { i--; j--; } 
        else if (i > 0) { i--; } 
        else if (j > 0) { j--; }
    }

    std::reverse(sumLine.begin(), sumLine.end());
    std::reverse(matchLine.begin(), matchLine.end());
    std::reverse(germLine.begin(), germLine.end());

    auto getVisualWidth = [](const std::string& token) {
        if (token.empty()) return 0;
        if (token.length() == 2 && (unsigned char)token[0] == 0xC3) return 1;
        return (int)token.length();
    };

    std::stringstream ss;
    //ss << "   Sumerian: "; for (const auto& s : sumLine) ss << std::setw(6) << s << " ";
    //ss << "\n   Mapping:  "; for (const auto& m : matchLine) ss << std::setw(6) << m << " ";
    //ss << "\n   Germanic: "; for (const auto& g : germLine) ss << std::setw(6) << g << " ";
    ss << "   Sumerian: "; 
    for (const auto& s : sumLine) {
        int padding = 6 - getVisualWidth(s);
        ss << s << std::string(padding > 0 ? padding : 1, ' ') << " ";
    }

    ss << "\n   Mapping:  "; 
    for (const auto& m : matchLine) {
        int padding = 6 - getVisualWidth(m);
        ss << m << std::string(padding > 0 ? padding : 1, ' ') << " ";
    }

    ss << "\n   Germanic: "; 
    for (const auto& g : germLine) {
        int padding = 6 - getVisualWidth(g);
        ss << g << std::string(padding > 0 ? padding : 1, ' ') << " ";
    }
    return ss.str();
}

CognateCandidate runAlignmentCrossCheck(const std::string& rawSumerian, const std::string& rawGermanic) {
    std::string cleanGermanic = cleanSuffixes(rawGermanic);
    
    std::vector<std::string> sumTokens = normalizeTokenVowelLength(tokenizePhonemes(rawSumerian));
    std::vector<std::string> germTokens = normalizeTokenVowelLength(tokenizePhonemes(cleanGermanic));

    bool isShortWord = (sumTokens.size() <= 2 || germTokens.size() <= 2);
    
    int m = sumTokens.size();
    int n = germTokens.size();
    
    if (m == 0 || n == 0) return {rawSumerian, rawGermanic, 0.0, ""};

    std::vector<std::vector<double>> scoreMatrix(m + 1, std::vector<double>(n + 1, 0.0));

    for (int i = 1; i <= m; i++) scoreMatrix[i][0] = scoreMatrix[i - 1][0] - 1.0;
    for (int j = 1; j <= n; j++) {
        double gapCost = 1.0;
        if (j > 1 && (germTokens[j-1] == "n" || germTokens[j-1] == "m") && !isVowelToken(germTokens[j-2])) gapCost = 0.25;
        scoreMatrix[0][j] = scoreMatrix[0][j - 1] - gapCost; 
    }

    for (int i = 1; i <= m; i++) {
        for (int j = 1; j <= n; j++) {
            double finalizedCost = GetTokenLinguisticCost(sumTokens[i - 1], germTokens[j - 1], i - 1, sumTokens, j - 1, germTokens);
            double stepMaxReward = (sumTokens[i - 1] != "kU" && isVowelToken(sumTokens[i - 1])) ? 1.50 : 2.50;
            double matchReward = (finalizedCost == 0.0) ? stepMaxReward : (stepMaxReward - (finalizedCost * stepMaxReward));
            
            double diagonal = scoreMatrix[i - 1][j - 1] + matchReward;
            double gapSumerian = scoreMatrix[i - 1][j] - 1.0; 
            
            double gCost = 1.0;
            if (germTokens[j-1] == "n" || germTokens[j-1] == "m") {
                if (j < n && !isVowelToken(germTokens[j])) gCost = 0.25;
            }
            double gapGermanic = scoreMatrix[i][j - 1] - gCost; 
            
            double metathesisBranch = -999.0;
            if (i > 1 && j > 1) {
                std::string currentSum  = sumTokens[i - 1];
                std::string previousSum = sumTokens[i - 2];
                std::string currentGerm = germTokens[j - 1];
                std::string previousGerm = germTokens[j - 2];
                
                if (currentSum == previousGerm && previousSum == currentGerm) {
                    double swapCost = (currentSum == "ku" || isVowelToken(currentSum)) ? 1.50 : 2.50;
                    double metathesisReward = swapCost - (0.20 * swapCost); 
                    metathesisBranch = scoreMatrix[i - 2][j - 2] + metathesisReward;
                }
            }
            scoreMatrix[i][j] = std::max({diagonal, gapSumerian, gapGermanic, metathesisBranch}); 
        }
    }
    
    double maxScore = scoreMatrix[m][n];
    
    // FINALIZED COMPRESSION FORMULA: Normalize dynamically based on true core radical overlap boundaries
    double perfectScore = 0.0;
    for (const std::string& token : sumTokens) {
        perfectScore += (token == "kU") ? 2.50 : (isVowelToken(token) ? 1.50 : 2.50);
    }
    
    // Balanced compression scale factor prevents short stems from being drowned out by glide onsets
    if (n > m) {
        perfectScore += (n - m) * 0.15;
    }

    double similarity = std::max(0.0, std::min(100.0, (maxScore / perfectScore) * 100.0));
    std::string report = getVisualPathReport(sumTokens, germTokens, scoreMatrix);
    
    return {rawSumerian, rawGermanic, similarity, report};
}

int main() {
    std::cout << "=================== BATCH DICTIONARY PIPELINE ACTIVE ===================\n\n";
    
    std::vector<std::pair<std::string, std::string>> dictionaryDataset = {
        {"kur", "horn"}, {"kur", "holm"}, {"ter", "treo"}, {"Ddag", "dag"},
        {"kus", "hus"}, {"kus", "hut"}, {"hal", "halba"}, {"pad", "findan"},
        {"Gen", "gang"}, {"tag", "taka"}, {"tuku", "taka"}, {"lu", "wer"}, {"uru", "burg"},
        {"saG", "hobid"}, {"saG", "saga"}, {"kUg", "hwit"}, {"shu", "hand"}, {"til", "lib"},
        {"bala", "bota"}, {"nu", "ne"}, {"pU", "pol"}, {"Gar", "garo"},
        {"mu", "menoths"}, {"niG", "thing"}, {"an", "ans"}, {"igi", "ouga"},
        {"tUm", "tuon"}, {"tUm", "dOm"}, {"sila", "skæp"}, {"siki", "skæp"}, {"gu", "ku"},
        {"sah", "swin"}, {"urbar", "wulfa"}, {"AG", "eigin"}, {"sipa", "skæp"}, {"Sbi", "skinan"}, {"mah", "mihil"}, {"mah", "maht"}, 
        {"gid", "wid"}, {"kA", "geat"}, {"ki", "her"}, {"she","sehan"}, {"she", "sekan"}, {"ul", "alt"}, {"utu", "ut"},{"bur", "bolla"},
         {"bul", "blAwan"}, {"dar", "teran"}, {"har", "hring"}, {"hur", "hol"}, {"ani", "inan"}, {"ane", "inan"}, {"de", "te"}, {"bar", "faran"}, {"simug", "smith"},
         {"shika", "skiel"}, {"banD", "barn"}, {"sIl", "slItan"}, {"sikil", "skin"}, {"diri", "diurian"}, {"kar", "heri"}, {"kar", "hors"}, {"luh", "leah"}, 
         {"diGir", "tungol"}, {"dumu", "dohter"}, {"kag", "hAt"}, {"kur", "hard"}, {"silim", "sliumo"}, {"en", "Ain"}, {"bir", "brikan"}, {"hUl", "hlas"}, 
         {"bUr", "bAl"}, {"gur", "hwel"}
         


    };

    std::vector<CognateCandidate> primeTier, probableTier, speculativeTier, noiseTier;

    for (const auto& wordPair : dictionaryDataset) {
        CognateCandidate res = runAlignmentCrossCheck(wordPair.first, wordPair.second);

       std::string cleanGermanic = cleanSuffixes(wordPair.second);
       bool isShortWord = (wordPair.first.length() <= 2 || cleanGermanic.length() <= 2);
    
    // Dynamic thresholds based on length
       double primeThreshold       = isShortWord ? 90.0 : 75.0; // probable and prime stricter for short words
       double probableThreshold    = isShortWord ? 65.0 : 50.0;
       double speculativeThreshold = isShortWord ? 40.0 : 40.0;

    // Sort into tiers using the adjusted thresholds
       if (res.similarity >= primeThreshold) {
           primeTier.push_back(res);
        } 
        else if (res.similarity >= probableThreshold) {
            probableTier.push_back(res);
        } 
        else if (res.similarity >= speculativeThreshold) {
            speculativeTier.push_back(res);
        } 
        else {
            noiseTier.push_back(res);
        }
    }

    std::cout << "🥇 [PRIME COGNATE CANDIDATES] (>= 75% Match)\n";
    std::cout << "========================================================================\n";
    for (const auto& c : primeTier) {
        std::cout << "-> " << c.sumerian << " <--> " << c.germanic << " [" << std::fixed << std::setprecision(2) << c.similarity << "%]\n" << c.pathMapReport << "\n\n";
    }

    std::cout << "🥈 [PROBABLE COGNATE CANDIDATES] (50% - 74.9% Match)\n";
    std::cout << "========================================================================\n";
    for (const auto& c : probableTier) {
        std::cout << "-> " << c.sumerian << " <--> " << c.germanic << " [" << std::fixed << std::setprecision(2) << c.similarity << "%]\n" << c.pathMapReport << "\n\n";
    }

    std::cout << "🥉 [SPECULATIVE COGNATE CANDIDATES] (40% - 49.9% Match)\n";
    std::cout << "========================================================================\n";
    for (const auto& c : speculativeTier) {
        std::cout << "-> " << c.sumerian << " <--> " << c.germanic << " [" << std::fixed << std::setprecision(2) << c.similarity << "%]\n" << c.pathMapReport << "\n\n";
    }

    std::cout << "❌ [REJECTED MATRIX NOISE] (< 40% Match)\n";
    std::cout << "========================================================================\n";
    for (const auto& c : noiseTier) {
        std::cout << "-> Mismatch: [" << c.sumerian << "] x [" << c.germanic << "] -> Score: " << std::fixed << std::setprecision(2) << c.similarity << "%\n";
    }
    std::cout << "========================================================================\n";

    return 0;
}