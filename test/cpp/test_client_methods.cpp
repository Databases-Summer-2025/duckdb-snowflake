#include <iostream>
#include <cstdlib>
#include "duckdb.hpp"
#include "snowflake_client.hpp"
#include "snowflake_client_manager.hpp"
#include "duckdb/main/database.hpp"
#include "duckdb/main/connection.hpp"

using namespace duckdb;
using namespace duckdb::snowflake;

int main() {
	try {
		// Get credentials from environment
		const char *account = std::getenv("SNOWFLAKE_TEST_ACCOUNT");
		const char *user = std::getenv("SNOWFLAKE_TEST_USER");
		const char *password = std::getenv("SNOWFLAKE_TEST_PASSWORD");
		const char *database = std::getenv("SNOWFLAKE_TEST_DATABASE");
		const char *warehouse = std::getenv("SNOWFLAKE_TEST_WAREHOUSE");

		if (!account || !user || !password) {
			std::cerr << "Error: Set SNOWFLAKE_TEST_* environment variables" << std::endl;
			return 1;
		}

		// Create connection string
		std::string conn_str = "account=" + std::string(account) + ";user=" + std::string(user) +
		                       ";password=" + std::string(password) +
		                       ";database=" + std::string(database ? database : "SNOWFLAKE_SAMPLE_DATA") +
		                       ";warehouse=" + std::string(warehouse ? warehouse : "COMPUTE_WH");

		std::cout << "Testing Snowflake client methods..." << std::endl;

		// Create DuckDB instance and context
		DuckDB db;
		Connection con(db);
		auto &context = *con.context;

		// Parse config and get connection
		auto config = SnowflakeConfig::ParseConnectionString(conn_str);
		auto &manager = SnowflakeClientManager::GetInstance();
		auto sf_conn = manager.GetConnection(conn_str, config);

		// Test ListSchemas
		std::cout << "\nTesting ListSchemas:" << std::endl;
		auto schemas = sf_conn->ListSchemas(context);
		std::cout << "Found " << schemas.size() << " schemas:" << std::endl;
		for (const auto &schema : schemas) {
			std::cout << "  - " << schema << std::endl;
		}

		// Test ListTables for PUBLIC schema
		std::cout << "\nTesting ListTables for PUBLIC schema:" << std::endl;
		auto tables = sf_conn->ListTables(context, "PUBLIC");
		std::cout << "Found " << tables.size() << " tables in PUBLIC:" << std::endl;
		for (size_t i = 0; i < std::min(tables.size(), size_t(5)); ++i) {
			std::cout << "  - " << tables[i] << std::endl;
		}
		if (tables.size() > 5) {
			std::cout << "  ... and " << (tables.size() - 5) << " more" << std::endl;
		}

		std::cout << "\nAll tests passed!" << std::endl;
		return 0;

	} catch (const std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
}
