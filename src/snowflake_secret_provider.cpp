#include "snowflake_secret_provider.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/serializer/deserializer.hpp"
#include "duckdb/common/serializer/serializer.hpp"

namespace duckdb {

//! Get Snowflake-specific fields
string SnowflakeSecret::GetUser() const {
	Value value;
	if (TryGetValue("user", value)) {
		return value.GetValue<string>();
	}
	return "";
}

string SnowflakeSecret::GetPassword() const {
	Value value;
	if (TryGetValue("password", value)) {
		return value.GetValue<string>();
	}
	return "";
}

string SnowflakeSecret::GetAccount() const {
	Value value;
	if (TryGetValue("account", value)) {
		return value.GetValue<string>();
	}
	return "";
}

string SnowflakeSecret::GetWarehouse() const {
	Value value;
	if (TryGetValue("warehouse", value)) {
		return value.GetValue<string>();
	}
	return "";
}

string SnowflakeSecret::GetDatabase() const {
	Value value;
	if (TryGetValue("database", value)) {
		return value.GetValue<string>();
	}
	return "";
}

string SnowflakeSecret::GetSchema() const {
	Value value;
	if (TryGetValue("schema", value)) {
		return value.GetValue<string>();
	}
	return "";
}

string SnowflakeSecret::GetAuthType() const {
	Value value;
	if (TryGetValue("auth_type", value)) {
		return value.GetValue<string>();
	}
	return "password";  // default
}

string SnowflakeSecret::GetToken() const {
	Value value;
	if (TryGetValue("token", value)) {
		return value.GetValue<string>();
	}
	return "";
}

string SnowflakeSecret::GetPrivateKey() const {
	Value value;
	if (TryGetValue("private_key", value)) {
		return value.GetValue<string>();
	}
	return "";
}

string SnowflakeSecret::GetRole() const {
	Value value;
	if (TryGetValue("role", value)) {
		return value.GetValue<string>();
	}
	return "";
}

string SnowflakeSecret::GetUsername() const {
	// Try 'username' first, then fall back to 'user'
	Value value;
	if (TryGetValue("username", value)) {
		return value.GetValue<string>();
	}
	return GetUser();  // Fall back to GetUser() which handles 'user' field
}

//! Validate that all required fields are present
void SnowflakeSecret::Validate() const {
	// Account is always required
	Value account_value;
	if (!TryGetValue("account", account_value) || account_value.IsNull()) {
		throw InvalidInputException("Snowflake secret is missing required field: account");
	}

	// Check auth type to determine required fields
	Value auth_type_value;
	string auth_type = "password";  // default
	if (TryGetValue("auth_type", auth_type_value) && !auth_type_value.IsNull()) {
		auth_type = auth_type_value.GetValue<string>();
	}

	vector<string> missing_fields;
	
	if (auth_type == "browser" || auth_type == "ext_browser") {
		// Browser auth only requires username
		Value username_value;
		if (!TryGetValue("username", username_value) || username_value.IsNull()) {
			missing_fields.push_back("username");
		}
	} else if (auth_type == "oauth") {
		// OAuth requires token
		Value token_value;
		if (!TryGetValue("token", token_value) || token_value.IsNull()) {
			missing_fields.push_back("token");
		}
	} else if (auth_type == "key_pair") {
		// Key pair requires user and private_key
		Value user_value, key_value;
		if (!TryGetValue("user", user_value) || user_value.IsNull()) {
			missing_fields.push_back("user");
		}
		if (!TryGetValue("private_key", key_value) || key_value.IsNull()) {
			missing_fields.push_back("private_key");
		}
	} else {
		// Password auth (default) requires user and password
		Value user_value, password_value;
		if (!TryGetValue("user", user_value) || user_value.IsNull()) {
			missing_fields.push_back("user");
		}
		if (!TryGetValue("password", password_value) || password_value.IsNull()) {
			missing_fields.push_back("password");
		}
	}

	if (!missing_fields.empty()) {
		throw InvalidInputException("Snowflake secret is missing required fields for auth_type '%s': %s",
		                            auth_type, StringUtil::Join(missing_fields, ", "));
	}
}

//! Custom serialization for Snowflake secrets
void SnowflakeSecret::Serialize(Serializer &serializer) const {
	// First serialize the base KeyValueSecret
	KeyValueSecret::Serialize(serializer);

	// Add any Snowflake-specific serialization if needed
	// For now, we just use the base KeyValueSecret serialization
}

//! Custom deserialization for Snowflake secrets
unique_ptr<BaseSecret> SnowflakeSecret::Deserialize(Deserializer &deserializer, BaseSecret base_secret) {
	auto result = make_uniq<SnowflakeSecret>(base_secret.GetScope(), base_secret.GetProvider(), base_secret.GetName());

	// Deserialize the secret map
	Value secret_map_value;
	deserializer.ReadProperty(201, "secret_map", secret_map_value);

	for (const auto &entry : ListValue::GetChildren(secret_map_value)) {
		auto kv_struct = StructValue::GetChildren(entry);
		result->secret_map[kv_struct[0].ToString()] = kv_struct[1];
	}

	// Deserialize the redact keys
	Value redact_set_value;
	deserializer.ReadProperty(202, "redact_keys", redact_set_value);
	for (const auto &entry : ListValue::GetChildren(redact_set_value)) {
		result->redact_keys.insert(entry.ToString());
	}

	return std::move(result);
}

//! Create function for Snowflake secrets
unique_ptr<BaseSecret> CreateSnowflakeSecret(ClientContext &context, CreateSecretInput &input) {
	// Create the secret with the provided scope and name
	auto secret = make_uniq<SnowflakeSecret>(input.scope, input.provider, input.name);

	// Account is always required
	auto account_it = input.options.find("account");
	if (account_it == input.options.end()) {
		throw InvalidInputException("Snowflake secret requires field 'account'");
	}
	secret->secret_map["account"] = account_it->second;

	// Check auth type to determine required/optional fields
	string auth_type = "password";  // default
	auto auth_type_it = input.options.find("auth_type");
	if (auth_type_it != input.options.end()) {
		auth_type = auth_type_it->second.GetValue<string>();
		secret->secret_map["auth_type"] = auth_type_it->second;
	}

	// Process auth-specific fields
	if (auth_type == "browser" || auth_type == "ext_browser") {
		// Browser auth requires username
		auto username_it = input.options.find("username");
		if (username_it == input.options.end()) {
			throw InvalidInputException("Snowflake browser auth requires field 'username'");
		}
		secret->secret_map["username"] = username_it->second;
	} else if (auth_type == "oauth") {
		// OAuth requires token
		auto token_it = input.options.find("token");
		if (token_it == input.options.end()) {
			throw InvalidInputException("Snowflake OAuth requires field 'token'");
		}
		secret->secret_map["token"] = token_it->second;
		// Optional username for OAuth
		auto username_it = input.options.find("username");
		if (username_it != input.options.end()) {
			secret->secret_map["username"] = username_it->second;
		}
	} else if (auth_type == "key_pair") {
		// Key pair requires user and private_key
		auto user_it = input.options.find("user");
		if (user_it == input.options.end()) {
			throw InvalidInputException("Snowflake key pair auth requires field 'user'");
		}
		secret->secret_map["user"] = user_it->second;
		
		auto key_it = input.options.find("private_key");
		if (key_it == input.options.end()) {
			throw InvalidInputException("Snowflake key pair auth requires field 'private_key'");
		}
		secret->secret_map["private_key"] = key_it->second;
	} else {
		// Password auth (default) requires user and password
		auto user_it = input.options.find("user");
		if (user_it == input.options.end()) {
			throw InvalidInputException("Snowflake password auth requires field 'user'");
		}
		secret->secret_map["user"] = user_it->second;
		
		auto password_it = input.options.find("password");
		if (password_it == input.options.end()) {
			throw InvalidInputException("Snowflake password auth requires field 'password'");
		}
		secret->secret_map["password"] = password_it->second;
	}

	// Process common optional fields
	vector<string> optional_fields = {"warehouse", "database", "schema", "role", "query_timeout", "keep_alive", "use_high_precision"};
	for (const auto &field : optional_fields) {
		auto it = input.options.find(field);
		if (it != input.options.end()) {
			secret->secret_map[field] = it->second;
		}
	}

	// Also handle 'username' as an alias for 'user' if not already set
	if (secret->secret_map.find("user") == secret->secret_map.end()) {
		auto username_it = input.options.find("username");
		if (username_it != input.options.end()) {
			secret->secret_map["user"] = username_it->second;
		}
	}

	// Validate the secret
	secret->Validate();

	return std::move(secret);
}

//! Register the Snowflake secret type with DuckDB
void RegisterSnowflakeSecretType(DatabaseInstance &instance) {
	auto &secret_manager = SecretManager::Get(instance);

	// Create the secret type
	SecretType snowflake_type;
	snowflake_type.name = "snowflake";
	snowflake_type.default_provider = "config";
	snowflake_type.extension = "snowflake";
	snowflake_type.deserializer = SnowflakeSecret::Deserialize;

	// Register the secret type
	secret_manager.RegisterSecretType(snowflake_type);

	// Create the create function
	CreateSecretFunction create_function;
	create_function.secret_type = "snowflake";
	create_function.provider = "config";
	create_function.function = CreateSnowflakeSecret;

	// Define the named parameters for the CREATE SECRET statement
	create_function.named_parameters["account"] = LogicalType::VARCHAR;
	create_function.named_parameters["auth_type"] = LogicalType::VARCHAR;
	create_function.named_parameters["user"] = LogicalType::VARCHAR;
	create_function.named_parameters["username"] = LogicalType::VARCHAR;  // Alias for user
	create_function.named_parameters["password"] = LogicalType::VARCHAR;
	create_function.named_parameters["token"] = LogicalType::VARCHAR;
	create_function.named_parameters["private_key"] = LogicalType::VARCHAR;
	create_function.named_parameters["warehouse"] = LogicalType::VARCHAR;
	create_function.named_parameters["database"] = LogicalType::VARCHAR;
	create_function.named_parameters["schema"] = LogicalType::VARCHAR;
	create_function.named_parameters["role"] = LogicalType::VARCHAR;
	create_function.named_parameters["query_timeout"] = LogicalType::INTEGER;
	create_function.named_parameters["keep_alive"] = LogicalType::BOOLEAN;
	create_function.named_parameters["use_high_precision"] = LogicalType::BOOLEAN;

	// Register the create function
	secret_manager.RegisterSecretFunction(create_function, OnCreateConflict::ERROR_ON_CONFLICT);
}

} // namespace duckdb
