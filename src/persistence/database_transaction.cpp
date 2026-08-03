#include "database_transaction.hpp"
#include <string>
#include <stdexcept>
#include <sqlite3.h>

#include "database.hpp"

namespace vcf
{

    DatabaseTransaction::DatabaseTransaction(Database &database) : m_database(database)
    {
        constexpr auto sql = "BEGIN TRANSACTION;";

        char *errorMessage = nullptr;

        const int rc = sqlite3_exec(
            m_database.connection(),
            sql,
            nullptr,
            nullptr,
            &errorMessage);

        if (rc != SQLITE_OK)
        {
            const std::string message = errorMessage ? errorMessage : "Unknown SQLite error";

            if (errorMessage)
                sqlite3_free(errorMessage);

            throw std::runtime_error("Failed to begin transaction: " + message);
        }
    }

    DatabaseTransaction::~DatabaseTransaction() noexcept
    {
        if (!m_active)
            return;

        constexpr auto sql = "ROLLBACK;";
        char *errorMessage = nullptr;

        sqlite3_exec(
            m_database.connection(),
            sql,
            nullptr,
            nullptr,
            &errorMessage);

        if (errorMessage)
            sqlite3_free(errorMessage);
    }

    void DatabaseTransaction::commit()
    {
        if (!m_active)
        {
            throw std::logic_error("Transaction is no longer active.");
        }

        constexpr auto sql = "COMMIT;";

        char *errorMessage = nullptr;

        const int rc = sqlite3_exec(
            m_database.connection(),
            sql,
            nullptr,
            nullptr,
            &errorMessage);

        if (rc != SQLITE_OK)
        {
            const std::string message = errorMessage ? errorMessage : "Unknown SQLite error";

            if (errorMessage)
                sqlite3_free(errorMessage);

            throw std::runtime_error("Failed to commit transaction: " + message);
        }
        m_active = false;
    }
}