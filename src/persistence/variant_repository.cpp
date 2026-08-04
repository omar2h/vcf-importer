#include "variant_repository.hpp"
#include <string>
#include <stdexcept>
#include <sqlite3.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "database.hpp"
#include "../domain/variant.hpp"
#include "../domain/vcf_header.hpp"

namespace vcf
{

    namespace
    {

        std::string serializeAlternateAlleles(const std::vector<std::string> &alternateAlleles)
        {
            std::string result;
            for (std::size_t i = 0; i < alternateAlleles.size(); ++i)
            {
                if (i != 0)
                    result += ",";
                result += alternateAlleles[i];
            }
            return result;
        }

        std::string serializeFailedFilters(const std::vector<std::string> &failedFilterIds)
        {
            std::string result;
            for (std::size_t i = 0; i < failedFilterIds.size(); ++i)
            {
                if (i != 0)
                    result += ";";
                result += failedFilterIds[i];
            }
            return result;
        }

        std::string serializeFieldValues(const std::vector<std::string> &entryValues)
        {
            std::string result;
            for (std::size_t i = 0; i < entryValues.size(); ++i)
            {
                if (i != 0)
                    result += ",";
                result += entryValues[i];
            }
            return result;
        }

        json serializeFieldValue(FieldType type, const std::vector<std::string> &values)
        {
            if (type == FieldType::Flag)
            {
                if (values.empty())
                    return true;

                throw std::logic_error("Flag field must not contain values");
            }

            if (values.empty())
                throw std::logic_error("Non-flag field must contain a value");

            if (values.size() > 1)
                return serializeFieldValues(values);

            const auto &value = values.front();
            if (value == ".")
                return nullptr;

            switch (type)
            {
            case FieldType::Integer:
                return std::stoi(value);
                break;
            case FieldType::Float:
                return std::stod(value);
                break;
            case FieldType::String:
                return value;
                break;
            case FieldType::Character:
                return value;
                break;
            }

            throw std::logic_error("Unsupported field type");
        }

        json serializeFilter(const Filter &filter)
        {
            switch (filter.status)
            {
            case FilterStatus::NotApplied:
                return nullptr;

            case FilterStatus::Passed:
                return "PASS";

            case FilterStatus::Failed:
                return serializeFailedFilters(filter.failedFilterIds);
            }
            throw std::logic_error("Unsupported filter status");
        }

        json serializeQuality(std::optional<double> quality)
        {
            if (!quality)
                return nullptr;
            return *quality;
        }

        json serializeInfo(const std::vector<InfoEntry> &infoEntries, const VcfHeader &header)
        {
            json info = json::object();

            for (const auto &entry : infoEntries)
            {
                FieldType type = header.infoDefinitions.at(entry.key).type;
                info[entry.key] = serializeFieldValue(type, entry.values);
            }
            return info;
        }

        json serializeFormat(const std::vector<Sample> &samples, const VcfHeader &header)
        {
            if (samples.size() != header.sampleNames.size())
                throw std::runtime_error("Sample count does not match VCF header sample names");

            json format = json::object();

            for (std::size_t i = 0; i < samples.size(); ++i)
            {
                const auto &sample = samples[i];
                const auto &sampleName = header.sampleNames[i];
                json sampleObject = json::object();

                for (const auto &entry : sample.formatEntries)
                {
                    FieldType type = header.formatDefinitions.at(entry.key).type;
                    sampleObject[entry.key] = serializeFieldValue(type, entry.values);
                }
                format[sampleName] = std::move(sampleObject);
            }
            return format;
        }

        std::string serializeVariantData(const Variant &variant, const VcfHeader &header)
        {
            json data;

            data["FILTER"] = serializeFilter(variant.filter);
            data["QUAL"] = serializeQuality(variant.quality);
            data["INFO"] = serializeInfo(variant.info, header);
            data["FORMAT"] = serializeFormat(variant.samples, header);

            return data.dump();
        }
    }

    VariantRepository::VariantRepository(Database &database) : m_database(database)
    {
    }

    VariantRepository::~VariantRepository() noexcept
    {
        if (m_insertStatement)
        {
            sqlite3_finalize(m_insertStatement);
        }
    }

    void VariantRepository::prepareInsertStatement()
    {
        constexpr auto sql = R"(
            INSERT INTO variants (
                chromosome,
                position,
                ref,
                alt,
                data
            )
            VALUES (?, ?, ?, ?, ?);
        )";

        const int rc = sqlite3_prepare_v2(m_database.connection(), sql, -1, &m_insertStatement, nullptr);

        if (rc != SQLITE_OK)
        {
            sqlite3_finalize(m_insertStatement);
            m_insertStatement = nullptr;

            const std::string message = sqlite3_errmsg(m_database.connection());
            throw std::runtime_error("Failed to prepare variant insert statement: " + message);
        }
    }

    void VariantRepository::resetInsertStatement() noexcept
    {
        sqlite3_reset(m_insertStatement);
        sqlite3_clear_bindings(m_insertStatement);
    }

    void VariantRepository::initializeSchema()
    {
        constexpr auto schema = R"(
            CREATE TABLE IF NOT EXISTS variants
            (
                id INTEGER PRIMARY KEY,
                chromosome TEXT NOT NULL,
                position INTEGER NOT NULL,
                ref TEXT NOT NULL,
                alt TEXT NOT NULL,
                data TEXT NOT NULL
            );
        )";

        char *errorMessage = nullptr;

        const int rc = sqlite3_exec(
            m_database.connection(),
            schema,
            nullptr,
            nullptr,
            &errorMessage);

        if (rc != SQLITE_OK)
        {
            const std::string message = errorMessage ? errorMessage : "Unknown SQLite error";

            if (errorMessage)
                sqlite3_free(errorMessage);

            throw std::runtime_error("Failed to initialize database schema: " + message);
        }

        if (!m_insertStatement)
            prepareInsertStatement();
    }

    void VariantRepository::createIndex()
    {
        constexpr auto schema = R"(
            CREATE INDEX IF NOT EXISTS idx_variants_chromosome_position
            ON variants(chromosome, position);
        )";

        char *errorMessage = nullptr;

        const int rc = sqlite3_exec(
            m_database.connection(),
            schema,
            nullptr,
            nullptr,
            &errorMessage);

        if (rc != SQLITE_OK)
        {
            const std::string message = errorMessage ? errorMessage : "Unknown SQLite error";

            if (errorMessage)
                sqlite3_free(errorMessage);

            throw std::runtime_error("Failed to initialize database schema: " + message);
        }
    }

    void VariantRepository::insert(const Variant &variant, const VcfHeader &header)
    {
        if (!m_insertStatement)
            throw std::logic_error("VariantRepository must be initialized before insert");

        const auto alternateAlleles = serializeAlternateAlleles(variant.alternateAlleles);
        const auto data = serializeVariantData(variant, header);

        const int bind1 = sqlite3_bind_text(m_insertStatement, 1, variant.chromosome.c_str(), -1, SQLITE_TRANSIENT);
        const int bind2 = sqlite3_bind_int64(m_insertStatement, 2, variant.position);
        const int bind3 = sqlite3_bind_text(m_insertStatement, 3, variant.referenceAllele.c_str(), -1, SQLITE_TRANSIENT);
        const int bind4 = sqlite3_bind_text(m_insertStatement, 4, alternateAlleles.c_str(), -1, SQLITE_TRANSIENT);
        const int bind5 = sqlite3_bind_text(m_insertStatement, 5, data.c_str(), -1, SQLITE_TRANSIENT);

        if (bind1 != SQLITE_OK || bind2 != SQLITE_OK || bind3 != SQLITE_OK || bind4 != SQLITE_OK || bind5 != SQLITE_OK)
        {
            const std::string message = "Failed to bind variant: " + std::string(sqlite3_errmsg(m_database.connection()));
            resetInsertStatement();
            throw std::runtime_error(message);
        }

        const int stepResult = sqlite3_step(m_insertStatement);

        if (stepResult != SQLITE_DONE)
        {
            const std::string message = "Failed to execute variant insert: " + std::string(sqlite3_errmsg(m_database.connection()));
            resetInsertStatement();
            throw std::runtime_error(message);
        }

        resetInsertStatement();
    }
}