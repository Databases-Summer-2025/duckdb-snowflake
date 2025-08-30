#include "catch.hpp"
#include "snowflake_client.hpp"
#include "duckdb.hpp"

using namespace duckdb;
using namespace duckdb::snowflake;

TEST_CASE("Test ExecuteAndGetChunk", "[snowflake]") {
	SnowflakeClient conn;
	SnowflakeConfig config;

	config.account = "your_test_account";
	config.username = "your_test_user";
	config.password = "your_test_password";

	conn.Connect(config);

	DuckDB db(nullptr);
	Connection duck_conn(db);
	ClientContext &context = *duck_conn.context;

	string query = "SELECT 1, 'hello';";
	vector<string> expected_names = {"1", "HELLO"};
	vector<LogicalType> expected_types = {LogicalType::INTEGER, LogicalType::VARCHAR};

	unique_ptr<DataChunk> chunk = conn.ExecuteAndGetChunk(context, query, expected_names, expected_types);

	REQUIRE(chunk != nullptr);
	REQUIRE(chunk->ColumnCount() == 2);
	REQUIRE(chunk->size() == 1);

	auto &col1 = chunk->data[0];
	auto &col2 = chunk->data[1];

	CHECK(col1.GetValue(0).GetValue<int32_t>() == 1);
	CHECK(col2.GetValue(0).ToString() == "hello");
}
