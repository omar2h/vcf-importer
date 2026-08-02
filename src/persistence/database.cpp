#include "database.hpp"
#include <sqlite3.h>

#include "../config/database_config.hpp"

namespace vcf
{

    Database::Database(const DatabaseConfig &config)
    {
        sqlite3 *connection{};
        int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
        int rc = sqlite3_open_v2(config.databasePath.c_str(), &connection, flags, nullptr);

        if (rc != SQLITE_OK)
        {
            std::string error{};
            if (connection)
            {
                error = sqlite3_errmsg(connection);
                sqlite3_close(connection);
            }

            throw std::runtime_error("Failed to open database " + config.databasePath.string() + ": " + error);
        }

        m_connection = connection;
    }

    Database::~Database()
    {
        if (m_connection)
            sqlite3_close(m_connection);
    }

    sqlite3 *Database::connection() const
    {
        return m_connection;
    }
}