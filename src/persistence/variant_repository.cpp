#include "variant_repository.hpp"
#include <string>
#include <stdexcept>
#include <sqlite3.h>

#include "database.hpp"
#include "../domain/variant.hpp"

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

    }

    VariantRepository::VariantRepository(Database &database) : m_database(database)
    {
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

    void VariantRepository::insert(const Variant &variant)
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

        sqlite3_stmt *statement = nullptr;
        const int rc = sqlite3_prepare_v2(m_database.connection(), sql, -1, &statement, nullptr);

        if (rc != SQLITE_OK)
        {
            sqlite3_finalize(statement);
            throw std::runtime_error("Failed to insert");
        }

        const auto alternateAlleles = serializeAlternateAlleles(variant.alternateAlleles);

        const int bind1 = sqlite3_bind_text(statement, 1, variant.chromosome.c_str(), -1, SQLITE_TRANSIENT);
        const int bind2 = sqlite3_bind_int64(statement, 2, variant.position);
        const int bind3 = sqlite3_bind_text(statement, 3, variant.referenceAllele.c_str(), -1, SQLITE_TRANSIENT);
        const int bind4 = sqlite3_bind_text(statement, 4, alternateAlleles.c_str(), -1, SQLITE_TRANSIENT);
        const int bind5 = sqlite3_bind_text(statement, 5, "{}", -1, SQLITE_TRANSIENT);

        if (bind1 != SQLITE_OK || bind2 != SQLITE_OK || bind3 != SQLITE_OK || bind4 != SQLITE_OK || bind5 != SQLITE_OK)
        {
            sqlite3_finalize(statement);
            throw std::runtime_error("Failed to insert");
        }

        const int stepResult = sqlite3_step(statement);

        if (stepResult != SQLITE_DONE)
        {
            sqlite3_finalize(statement);
            throw std::runtime_error("Failed to insert");
        }

        sqlite3_finalize(statement);
    }
}