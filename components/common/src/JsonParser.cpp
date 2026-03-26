#include "JsonParser.hpp"

#include <cJSON.h>
#include <esp_log.h>

#include <cctype>
#include <utility>
#include <vector>

namespace common {
namespace {
constexpr const char* Tag = "JsonParser";
constexpr const char* VersionKey = "\"version\"";

static size_t skipWs(const std::string& s, size_t pos) {
    while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos])) != 0) {
        ++pos;
    }
    return pos;
}
}  // namespace

bool JsonParser::parseStations(const std::string& serialized,
                               std::vector<common::StationData>& outStations) {
    if (serialized.empty()) {
        ESP_LOGE(Tag, "Input JSON is empty");
        return false;
    }

    // Build parsed JSON tree once, then walk only expected fields.
    cJSON* root = cJSON_ParseWithLength(serialized.c_str(), serialized.size());
    if (!root) {
        const char* errPtr = cJSON_GetErrorPtr();
        ESP_LOGE(Tag, "Failed to parse JSON near: %s", errPtr ? errPtr : "<unknown>");
        return false;
    }

    if (!cJSON_IsArray(root)) {
        ESP_LOGE(Tag, "Root JSON node must be an array");
        cJSON_Delete(root);
        return false;
    }

    std::vector<common::StationData> parsed;
    const int count = cJSON_GetArraySize(root);
    parsed.reserve(static_cast<size_t>(count > 0 ? count : 0));

    for (int i = 0; i < count; ++i) {
        cJSON* item = cJSON_GetArrayItem(root, i);
        if (!item || !cJSON_IsObject(item)) {
            ESP_LOGW(Tag, "Skipping station[%d]: not an object", i);
            continue;
        }

        cJSON* id = cJSON_GetObjectItemCaseSensitive(item, "id");
        cJSON* name = cJSON_GetObjectItemCaseSensitive(item, "name");
        cJSON* url = cJSON_GetObjectItemCaseSensitive(item, "url");

        if (!cJSON_IsString(id) || !id->valuestring || !cJSON_IsString(name) ||
            !name->valuestring || !cJSON_IsString(url) || !url->valuestring) {
            ESP_LOGW(Tag, "Skipping station[%d]: missing/invalid id|name|url", i);
            continue;
        }

        parsed.push_back(common::StationData{
            .id = id->valuestring,
            .name = name->valuestring,
            .url = url->valuestring,
        });
    }

    cJSON_Delete(root);

    if (parsed.empty()) {
        ESP_LOGE(Tag, "No valid stations parsed from JSON");
        return false;
    }

    outStations = std::move(parsed);
    return true;
}

// TODO: Improve
bool JsonParser::parseManifest(const std::string& serialized, common::ManifestData& outManifest) {
    if (serialized.empty()) {
        ESP_LOGE(Tag, "Input manifest is empty");
        return false;
    }

    const size_t keyPos = serialized.find(VersionKey);
    if (keyPos == std::string::npos) {
        ESP_LOGE(Tag, "Missing 'version' key");
        return false;
    }

    const size_t colonPos = serialized.find(':', keyPos + 1U);
    if (colonPos == std::string::npos) {
        ESP_LOGE(Tag, "Missing ':' after 'version'");
        return false;
    }

    size_t valuePos = skipWs(serialized, colonPos + 1U);
    if (valuePos >= serialized.size()) {
        ESP_LOGE(Tag, "Missing version value");
        return false;
    }

    std::string parsedVersion;
    if (serialized[valuePos] == '"') {
        ++valuePos;
        const size_t endQuote = serialized.find('"', valuePos);
        if (endQuote == std::string::npos || endQuote == valuePos) {
            ESP_LOGE(Tag, "Invalid quoted version");
            return false;
        }
        parsedVersion = serialized.substr(valuePos, endQuote - valuePos);
    } else {
        const size_t numberStart = valuePos;
        while (valuePos < serialized.size()) {
            const char ch = serialized[valuePos];
            if ((ch >= '0' && ch <= '9') || ch == '.' || ch == '-' || ch == '+') {
                ++valuePos;
                continue;
            }
            break;
        }

        if (valuePos == numberStart) {
            ESP_LOGE(Tag, "Unsupported version value");
            return false;
        }
        parsedVersion = serialized.substr(numberStart, valuePos - numberStart);
    }

    if (parsedVersion.empty()) {
        ESP_LOGE(Tag, "Version cannot be empty");
        return false;
    }

    outManifest.version = std::move(parsedVersion);
    return true;
}

}  // namespace common
