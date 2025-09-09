# Snowflake Authentication Examples

This document provides examples of different authentication methods supported by the DuckDB Snowflake extension.

## Authentication Methods

The Snowflake extension supports four authentication methods:

1. **Password Authentication** (default)
2. **Browser-Based SSO Authentication** 
3. **OAuth Token Authentication**
4. **Key Pair Authentication**

## 1. Password Authentication

Traditional username/password authentication.

```sql
-- Create password-based secret
CREATE SECRET snowflake_password (
    TYPE snowflake,
    ACCOUNT 'myaccount.us-east-1',
    USER 'myusername',
    PASSWORD 'mypassword',
    WAREHOUSE 'COMPUTE_WH',
    DATABASE 'MYDB',
    ROLE 'MYROLE'
);

-- Use the secret to connect
ATTACH 'snowflake_password' AS sf (TYPE SNOWFLAKE);
```

## 2. Browser-Based SSO Authentication

Opens your browser for interactive authentication. Supports SAML, OAuth, MFA, and other SSO methods configured in Snowflake.

```sql
-- Create browser-based auth secret
CREATE SECRET snowflake_sso (
    TYPE snowflake,
    ACCOUNT 'myaccount.us-east-1',
    USERNAME 'user@company.com',
    AUTH_TYPE 'browser',
    WAREHOUSE 'COMPUTE_WH',
    DATABASE 'MYDB'
);

-- Use the secret (browser will open for authentication)
ATTACH 'snowflake_sso' AS sf (TYPE SNOWFLAKE);
```

### How Browser Authentication Works
1. When you execute the ATTACH command, your default browser opens
2. You authenticate through Snowflake's web interface
3. After successful authentication, the browser can be closed
4. The connection is established automatically

**Note**: This is ideal for interactive sessions but not suitable for automated scripts or CI/CD pipelines.

## 3. OAuth Token Authentication

For service accounts and automated workflows using pre-obtained OAuth tokens.

```sql
-- Create OAuth token secret
CREATE SECRET snowflake_oauth (
    TYPE snowflake,
    ACCOUNT 'myaccount.us-east-1',
    AUTH_TYPE 'oauth',
    TOKEN 'your_oauth_access_token_here',
    WAREHOUSE 'COMPUTE_WH',
    DATABASE 'MYDB'
);

-- Use the secret
ATTACH 'snowflake_oauth' AS sf (TYPE SNOWFLAKE);
```

### Obtaining OAuth Tokens
OAuth tokens must be obtained through Snowflake's OAuth flow. This typically involves:
1. Registering an OAuth client with Snowflake
2. Implementing the OAuth authorization flow
3. Exchanging authorization codes for access tokens
4. Managing token refresh (tokens typically expire)

## 4. Key Pair Authentication

Uses RSA key pairs for authentication, commonly used for service accounts.

```sql
-- Create key pair auth secret
CREATE SECRET snowflake_keypair (
    TYPE snowflake,
    ACCOUNT 'myaccount.us-east-1',
    USER 'service_account',
    AUTH_TYPE 'key_pair',
    PRIVATE_KEY 'MIIEvQIBADANBgkqhkiG9w0BAQEFA...(your private key)...',
    WAREHOUSE 'COMPUTE_WH',
    DATABASE 'MYDB'
);

-- Use the secret
ATTACH 'snowflake_keypair' AS sf (TYPE SNOWFLAKE);
```

### Setting Up Key Pair Authentication
1. Generate an RSA key pair:
   ```bash
   openssl genrsa 2048 | openssl pkcs8 -topk8 -inform PEM -out rsa_key.p8 -nocrypt
   openssl rsa -in rsa_key.p8 -pubout -out rsa_key.pub
   ```

2. Configure the public key in Snowflake:
   ```sql
   ALTER USER service_account SET RSA_PUBLIC_KEY='MIIBIjANBgkqh...';
   ```

3. Use the private key in the secret (without header/footer lines)

## Connection String Alternative

You can also use connection strings directly without creating secrets:

```sql
-- Password auth
ATTACH 'account=myaccount;user=myuser;password=mypass;warehouse=COMPUTE_WH;database=MYDB' 
    AS sf (TYPE SNOWFLAKE);

-- Browser auth
ATTACH 'account=myaccount;username=user@company.com;auth_type=browser;warehouse=COMPUTE_WH' 
    AS sf (TYPE SNOWFLAKE);

-- OAuth
ATTACH 'account=myaccount;auth_type=oauth;token=your_token;warehouse=COMPUTE_WH' 
    AS sf (TYPE SNOWFLAKE);

-- Key pair
ATTACH 'account=myaccount;user=service;auth_type=key_pair;private_key=your_key;warehouse=COMPUTE_WH' 
    AS sf (TYPE SNOWFLAKE);
```

## Optional Parameters

All authentication methods support these optional parameters:

- `WAREHOUSE`: Snowflake warehouse to use
- `DATABASE`: Default database
- `SCHEMA`: Default schema
- `ROLE`: Snowflake role to assume
- `QUERY_TIMEOUT`: Query timeout in seconds (default: 300)
- `KEEP_ALIVE`: Keep connection alive (default: true)
- `USE_HIGH_PRECISION`: Use high precision for decimals (default: true)

## Managing Secrets

```sql
-- List all Snowflake secrets
SELECT name, type FROM duckdb_secrets() WHERE type = 'snowflake';

-- Drop a secret
DROP SECRET snowflake_sso;

-- Update a secret (drop and recreate)
DROP SECRET IF EXISTS snowflake_password;
CREATE SECRET snowflake_password (...);
```

## Best Practices

1. **Interactive Sessions**: Use browser authentication for better security and convenience
2. **Automated Workflows**: Use key pair authentication for service accounts
3. **Temporary Access**: Use OAuth tokens with proper refresh mechanisms
4. **Production**: Never hardcode credentials; always use secrets or environment variables
5. **Security**: Regularly rotate credentials and use the principle of least privilege

## Troubleshooting

### Browser Authentication Issues
- Ensure your default browser is properly configured
- Check if popup blockers are interfering
- Verify your Snowflake account supports SSO

### Connection Failures
- Verify account format (e.g., `myaccount.us-east-1`)
- Check network connectivity and firewall rules
- Ensure the user has proper permissions in Snowflake
- For key pair auth, verify the public key is correctly configured in Snowflake

### Token Expiration
- OAuth tokens expire; implement refresh logic for production use
- Browser auth sessions also expire but are automatically refreshed when needed