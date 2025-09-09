# Snowflake OAuth Implementation Plan

## Overview
This document outlines the plan for implementing OAuth authentication in the DuckDB Snowflake extension, including both browser-based SSO and programmatic token management.

## Current State
- Basic OAuth token support exists (`oauth_token` field in `SnowflakeConfig`)
- Token is passed directly to ADBC driver via `adbc.snowflake.sql.auth_token`
- No token refresh mechanism
- No browser-based OAuth flow support

## Two OAuth Approaches

### A. Interactive Browser-Based (auth_ext_browser)
**For human users with interactive sessions**

`auth_ext_browser` is Snowflake's built-in browser-based Single Sign-On (SSO) authentication method that's already supported by the ADBC driver.

#### How It Works
1. **Automatic Browser Launch**: Opens default browser automatically
2. **Interactive Login**: User logs in through Snowflake's web interface (supports SAML, OAuth, MFA)
3. **Token Exchange**: Browser receives a token and passes it back to the application
4. **Blocking Operation**: Connection waits until authentication completes

#### Benefits
- **Zero token management** - ADBC handles everything
- **MFA support** - Works with all Snowflake authentication methods
- **No client secrets** - More secure for end users
- **Already implemented** in ADBC driver

### B. Programmatic Token-Based
**For service accounts, CI/CD, and automated workflows**

Uses pre-obtained OAuth tokens that need manual refresh management.

## Implementation Plan

### 1. Add Browser Auth Support to Config

#### Update `snowflake_config.hpp`
```cpp
enum class SnowflakeAuthType { 
    PASSWORD, 
    OAUTH,           // Manual token management
    KEY_PAIR,
    EXT_BROWSER      // NEW: Browser-based SSO
};
```

### 2. Update Connection Logic

#### In `snowflake_client.cpp` (lines 184-205), add:
```cpp
case SnowflakeAuthType::EXT_BROWSER:
    status = AdbcDatabaseSetOption(&database, "adbc.snowflake.sql.auth_type", 
                                  "auth_ext_browser", &error);
    CheckError(status, "Failed to set browser auth type", &error);
    break;
```

### 3. Update Config Parser

#### In `snowflake_config.cpp` (lines 38-45), add:
```cpp
else if (value == "browser" || value == "ext_browser") {
    config.auth_type = SnowflakeAuthType::EXT_BROWSER;
}
```

### 4. Enhanced OAuth Token Management (Future)

For programmatic OAuth, implement token refresh mechanism:

```cpp
struct SnowflakeConfig {
    // Existing OAuth field
    std::string oauth_token;
    
    // New OAuth fields for refresh
    std::string oauth_client_id;
    std::string oauth_client_secret;
    std::string oauth_refresh_token;
    int64_t oauth_token_expiry;  // Unix timestamp
    std::string oauth_scope;      // Default: "session:role-any"
};

class OAuthTokenManager {
    bool IsTokenExpired(const SnowflakeConfig& config);
    std::string RefreshAccessToken(const SnowflakeConfig& config);
    void UpdateStoredToken(ClientContext& context, 
                          const std::string& profile, 
                          const std::string& new_token, 
                          int64_t expiry);
};
```

### 5. OAuth Secret Provider Enhancement

```cpp
class SnowflakeOAuthSecret : public SnowflakeSecret {
    // Store OAuth-specific credentials
    std::string client_id;
    std::string client_secret;
    std::string refresh_token;
    std::string access_token;
    int64_t token_expiry;
};
```

## SQL Interface

### Browser-Based Authentication (Interactive)
```sql
-- Create browser-based SSO secret
CREATE SECRET snowflake_sso (
    TYPE snowflake,
    ACCOUNT 'myaccount',
    USERNAME 'user@company.com',
    AUTH_TYPE 'browser'
);

-- Use the secret
ATTACH 'snowflake_sso' AS sf (TYPE SNOWFLAKE);
```

### Token-Based Authentication (Programmatic)
```sql
-- Direct OAuth token for service accounts
CREATE SECRET snowflake_oauth (
    TYPE snowflake,
    ACCOUNT 'myaccount',
    AUTH_TYPE 'oauth',
    TOKEN 'your_oauth_token_here'
);

-- Future: OAuth with refresh capability
CREATE SECRET snowflake_oauth_refresh (
    TYPE snowflake,
    ACCOUNT 'myaccount',
    AUTH_TYPE 'oauth',
    CLIENT_ID 'your_client_id',
    CLIENT_SECRET 'your_client_secret',
    REFRESH_TOKEN 'your_refresh_token'
);
```

## Connection Flow

### Browser-Based Flow
1. User creates secret with `AUTH_TYPE 'browser'`
2. On connection, ADBC opens browser
3. User completes authentication
4. ADBC handles token exchange
5. Connection established

### Token-Based Flow
1. Check if token exists and is valid
2. If expired, attempt refresh using refresh_token
3. If no refresh_token, return error
4. Store new tokens in secrets manager
5. Use access_token for connection

## Implementation Priority

### Phase 1: Browser-Based SSO (Immediate)
1. ✅ Add `EXT_BROWSER` auth type enum
2. ✅ Update client connection logic 
3. ✅ Update config parser
4. ⬜ Test browser auth flow
5. ⬜ Update documentation

### Phase 2: Enhanced Token Management (Future)
1. ⬜ Add OAuth refresh fields to config
2. ⬜ Implement token refresh logic
3. ⬜ Create OAuth-specific secret type
4. ⬜ Add token expiry handling
5. ⬜ Write comprehensive tests

## Error Handling

### Browser Authentication Errors
- Browser launch failures
- User cancellation
- Network timeouts
- Invalid account/username

### Token Authentication Errors
- Invalid/expired tokens
- Network failures during refresh
- Missing OAuth configuration
- Authorization failures

## Security Considerations

1. **Token Storage**
   - Store tokens encrypted in secrets manager
   - Never log tokens or secrets
   - Clear tokens on disconnect

2. **Browser Authentication**
   - Use system default browser
   - Support corporate proxy settings
   - Handle popup blockers

3. **Token Refresh**
   - Implement token rotation
   - Use secure HTTPS connections
   - Validate token signatures

## Testing Strategy

### Unit Tests
- Config parsing with new auth types
- Token expiry detection
- Secret storage/retrieval

### Integration Tests
- Browser auth flow (manual)
- Token refresh mechanism
- Connection pooling with OAuth
- Error scenarios

### Manual Testing
- Different browsers
- Corporate SSO/SAML
- MFA scenarios
- Token expiration

## Documentation Updates

1. **User Guide**
   - How to use browser authentication
   - Setting up OAuth for service accounts
   - Troubleshooting auth issues

2. **API Documentation**
   - New auth_type values
   - OAuth-specific parameters
   - Secret creation examples

3. **Examples**
   - Interactive notebook usage
   - CI/CD pipeline setup
   - Service account configuration

## References

- [ADBC Snowflake Driver Documentation](https://arrow.apache.org/adbc/main/driver/snowflake.html)
- [Snowflake OAuth Documentation](https://docs.snowflake.com/en/user-guide/oauth)
- [DuckDB Secrets Manager](https://duckdb.org/docs/configuration/secrets_manager)

## Notes

The `auth_ext_browser` approach significantly simplifies OAuth implementation as the ADBC driver handles all the complexity of browser-based authentication, token exchange, and session management. This should be the preferred method for interactive user authentication, while programmatic token management should be reserved for service accounts and automated workflows.