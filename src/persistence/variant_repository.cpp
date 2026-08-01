#include "variant_repository.hpp"
#include <string>
#include <stdexcept>
#include <sqlite3.h>

#include "database.hpp"

namespace vcf
{
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
}